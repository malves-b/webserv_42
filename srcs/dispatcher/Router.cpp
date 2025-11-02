#include <cstring>
#include <algorithm>
#include <sys/stat.h>
#include <unistd.h>
#include <dispatcher/Router.hpp>
#include <response/ResponseStatus.hpp>
#include <config/ServerConfig.hpp>
#include <utils/Logger.hpp>
#include <utils/string_utils.hpp>

void	Router::resolve(HttpRequest& req, HttpResponse& res, const ServerConfig& config)
{
	// Handle parser-level error first
	if (req.getParseError() != ResponseStatus::OK)
	{
		Logger::instance().log(WARNING, "Router: Request parse error detected");
		req.setRouteType(RouteType::Error);
		res.setStatusCode(req.getParseError());
		return ;
	}

	// Match the location for this request URI
	const LocationConfig& location = config.mathLocation(req.getUri());
	const std::string index = location.getHasIndexFiles() ? location.getIndex() : "";
	const std::string root  = location.getHasRoot() ? location.getRoot() : config.getRoot();
	const std::string cgiPath = location.getCgiPath();

	// Path traversal guard
	if (hasParentTraversal(req.getUri()))
	{
		Logger::instance().log(WARNING, "Router: Path traversal attempt blocked: " + req.getUri());
		req.setRouteType(RouteType::Error);
		res.setStatusCode(ResponseStatus::Forbidden);
		return ;
	}

	// Build the resolved filesystem path
	computeResolvedPath(req, root);
	Logger::instance().log(DEBUG, "Router: Resolved path → " + req.getResolvedPath());

	ResponseStatus::code status = ResponseStatus::OK;

	if (isRedirect(req, res, config))
	{
		Logger::instance().log(INFO, "Router: Route type = Redirect");
		req.setRouteType(RouteType::Redirect);
		return ;
	}

	if (isUpload(req, res, config))
	{
		Logger::instance().log(INFO, "Router: Route type = Upload");
		req.setRouteType(RouteType::Upload);
		return ;
	}
	if (checkErrorStatus(status, req, res))
		return ;

	if (!index.empty() && isAutoIndex(index, req, config))
	{
		Logger::instance().log(INFO, "Router: Route type = AutoIndex");
		req.setRouteType(RouteType::AutoIndex);
		return ;
	}

	if (!index.empty() && isStaticFile(index, status, req))
	{
		Logger::instance().log(INFO, "Router: Route type = StaticPage");

		if (req.getMethod() != RequestMethod::GET)
		{
			Logger::instance().log(WARNING, "Router: Static file requested with invalid method");
			req.setRouteType(RouteType::Error);
			res.setStatusCode(ResponseStatus::MethodNotAllowed);
			return ;
		}

		req.setRouteType(RouteType::StaticPage);
		return ;
	}
	if (checkErrorStatus(status, req, res))
		return ;

	if (isCgi(cgiPath, req.getResolvedPath(), status))
	{
		Logger::instance().log(INFO, "Router: Route type = CGI");
		req.setRouteType(RouteType::CGI);
		return ;
	}
	if (checkErrorStatus(status, req, res))
		return ;

	Logger::instance().log(WARNING, "Router: No matching route found (404)");
	res.setStatusCode(ResponseStatus::NotFound);
	req.setRouteType(RouteType::Error);
}

void	Router::computeResolvedPath(HttpRequest& req, const std::string& rawRoot)
{
	const std::string& uri = req.getUri();
	req.setResolvedPath(joinPaths(rawRoot, uri));
}

bool	Router::checkErrorStatus(ResponseStatus::code status, HttpRequest& req, HttpResponse& res)
{
	if (status != ResponseStatus::OK)
	{
		Logger::instance().log(DEBUG, "Router: Error status detected (" + toString(status) + ")");
		req.setRouteType(RouteType::Error);
		res.setStatusCode(status);
		return (true);
	}
	return (false);
}

bool	Router::isRedirect(HttpRequest& req, HttpResponse& res, const ServerConfig& config)
{
	const LocationConfig& location = config.mathLocation(req.getUri());
	const std::pair<int, std::string> redirect = location.getReturn();

	if (redirect.first)
	{
		Logger::instance().log(DEBUG, "Router::isRedirect → " + redirect.second);
		req.getMeta().setRedirect(true);
		res.setChunked(false);
		res.addHeader("Location", redirect.second);
		res.setStatusCode(static_cast<ResponseStatus::code>(redirect.first));
		return (true);
	}
	return (false);
}

bool Router::isUpload(HttpRequest& req, HttpResponse& res, const ServerConfig& config)
{
	const LocationConfig& location = config.mathLocation(req.getUri());
	const std::string uploadPath = location.getUploadPath();
	const std::string& uri = req.getUri();

	Logger::instance().log(DEBUG, "Router::isUpload comparing uri=" + uri + " uploadPath=" + uploadPath);

	// Check if the URI matches the configured upload path
	if (uri == uploadPath &&
		(req.getMethod() == RequestMethod::POST || req.getMethod() == RequestMethod::PUT))
	{
		// Upload location exists, but uploads are not enabled
		if (!location.getUploadEnabled())
		{
			Logger::instance().log(WARNING, "Router::isUpload disabled for this location (403)");
			res.setStatusCode(ResponseStatus::Forbidden);
			req.setRouteType(RouteType::Error);
			return (false);
		}

		// If the path exists but is not writable, also return 403
		struct stat s;
		if (stat(uploadPath.c_str(), &s) == 0)
		{
			if (access(uploadPath.c_str(), W_OK) != 0)
			{
				Logger::instance().log(WARNING , "Router::isUpload path not writable: " + uploadPath);
				res.setStatusCode(ResponseStatus::Forbidden);
				req.setRouteType(RouteType::Error);
				return (false);
			}
		}

		return (true);
	}

	return (false);
}

bool Router::isAutoIndex(const std::string& index, HttpRequest& req, const ServerConfig& config)
{
	const LocationConfig& location = config.mathLocation(req.getUri());

	bool autoIndexEnabled = config.getAutoindex();

	// The location-level directive overrides the global one
	if (location.getHasAutoIndex())
		autoIndexEnabled = location.getAutoindex();

	if (!autoIndexEnabled)
		return (false);

	std::string path = req.getResolvedPath();
	struct stat s;

	// Check if it's a directory and the index file doesn't exist
	if (stat(path.c_str(), &s) == 0 && S_ISDIR(s.st_mode))
	{
		if (path[path.length() - 1] != '/')
			path += '/';

		path += index;

		struct stat sIndex;
		if (stat(path.c_str(), &sIndex) != 0 || !S_ISREG(sIndex.st_mode))
		{
			Logger::instance().log(DEBUG, "Router::isAutoIndex enabled for directory: " + req.getResolvedPath());
			return (true);
		}
	}

	return (false);
}


bool	Router::isStaticFile(const std::string& index, ResponseStatus::code& status, HttpRequest& req)
{
	Logger::instance().log(DEBUG, "Router::isStaticFile start");
	std::string path = req.getResolvedPath();

	struct stat s;
	if (stat(path.c_str(), &s) == 0 && S_ISDIR(s.st_mode))
	{
		Logger::instance().log(DEBUG, "Router::isStaticFile detected directory");

		if (path[path.length() - 1] != '/')
			path += '/';

		path += index;

		Logger::instance().log(DEBUG, "Router::isStaticFile probing index: " + path);

		if (stat(path.c_str(), &s) != 0)
			return (false);
	}

	if (stat(path.c_str(), &s) == 0 && S_ISREG(s.st_mode))
	{
		if (access(path.c_str(), R_OK) == 0)
		{
			if (hasCgiExtension(path))
				return (false);

			req.setResolvedPath(path);
			return (true);
		}

		Logger::instance().log(WARNING, "Router::isStaticFile forbidden access to " + path);
		status = ResponseStatus::Forbidden;
	}
	return (false);
}

bool	Router::isCgi(const std::string& cgiPath, const std::string resolvedPath, ResponseStatus::code& status)
{
	struct stat s;

	if (stat(resolvedPath.c_str(), &s) != 0 || !S_ISREG(s.st_mode))
		return (false);

	if (access(resolvedPath.c_str(), X_OK) != 0)
	{
		Logger::instance().log(WARNING, "Router::isCgi file not executable: " + resolvedPath);
		status = ResponseStatus::Forbidden;
		return (false);
	}

	if (!cgiPath.empty() && resolvedPath.find(cgiPath) != std::string::npos)
		return (true);

	return (hasCgiExtension(resolvedPath));
}

bool	Router::hasCgiExtension(const std::string& path)
{
	static const char* cgiExts[] = { ".cgi", ".pl", ".py" };
	std::string::size_type dotPos = path.rfind('.');

	if (dotPos == std::string::npos)
		return (false);

	std::string ext = path.substr(dotPos);
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	for (size_t i = 0; i < sizeof(cgiExts) / sizeof(cgiExts[0]); ++i)
	{
		if (ext == cgiExts[i])
			return (true);
	}
	return (false);
}
