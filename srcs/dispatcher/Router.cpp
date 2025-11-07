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
	const LocationConfig& loc = config.matchLocation(req.getUri());
	const std::string index = loc.getHasIndexFiles() ? loc.getIndex() : "";
	const std::string root  = loc.getHasRoot() ? loc.getRoot() : config.getRoot();
	const std::string cgiPath = loc.getCgiPath();

	// Path traversal guard
	if (hasParentTraversal(req.getUri()))
	{
		Logger::instance().log(WARNING, "Router: Path traversal attempt blocked: " + req.getUri());
		req.setRouteType(RouteType::Error);
		res.setStatusCode(ResponseStatus::Forbidden);
		return ;
	}

	// Build the resolved filesystem path
	computeResolvedPath(req, loc, config);

	if (req.getParseError() != ResponseStatus::OK
		&& req.getMethod() == RequestMethod::DELETE)
	{
		req.setRouteType(RouteType::Delete);
		return ;
	}

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
	if (checkErrorStatus(req, res))
		return ;

	if (index.empty() && isAutoIndex(index, req, config))
	{
		Logger::instance().log(INFO, "Router: Route type = AutoIndex");
		req.setRouteType(RouteType::AutoIndex);
		return ;
	}

	if (isCgi(loc, req, res))
	{
		Logger::instance().log(INFO, "Router: Route type = CGI");
		req.setRouteType(RouteType::CGI);
		return ;
	}

	if (checkErrorStatus(req, res))
		return ;

	if (!index.empty() && isStaticFile(index, req, res))
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
	if (checkErrorStatus(req, res))
		return ;

	Logger::instance().log(WARNING, "Router: No matching route found (404)");
	res.setStatusCode(ResponseStatus::NotFound);
	req.setRouteType(RouteType::Error);
}

void Router::computeResolvedPath(HttpRequest& req,
								const LocationConfig& location,
								const ServerConfig& config)
{
	std::string uri = req.getUri();
	std::string root;

	if (!location.getCgiPath().empty())
		root = location.getCgiPath();
	else if (location.getHasRoot())
		root = location.getRoot();
	else
		root = config.getRoot();

	std::string locPath = location.getPath();
	if (locPath.size() > 1 && locPath[locPath.size() - 1] == '/')
		locPath.erase(locPath.size() - 1);

	std::string relativePath = uri;
	if (uri.find(locPath) == 0)
		relativePath = uri.substr(locPath.length());
	if (!relativePath.empty() && relativePath[0] == '/')
		relativePath.erase(0, 1);

	std::string resolved = joinPaths(root, relativePath);

	struct stat s;
	if (stat(resolved.c_str(), &s) == 0 && S_ISDIR(s.st_mode))
	{
		if (location.getHasIndexFiles())
			resolved = joinPaths(resolved, location.getIndex());
	}

	req.setResolvedPath(resolved);

	Logger::instance().log(DEBUG,
		"Router::computeResolvedPath: uri=" + uri +
		" locPath=" + locPath +
		" root=" + root +
		" -> resolved=" + resolved);
}


bool	Router::checkErrorStatus(HttpRequest& req, HttpResponse& res)
{
	if (res.getStatusCode() != ResponseStatus::OK)
	{
		Logger::instance().log(DEBUG, "Router: Error status detected (" + toString(res.getStatusCode()) + ")");
		req.setRouteType(RouteType::Error);
		return (true);
	}
	return (false);
}

bool	Router::isRedirect(HttpRequest& req, HttpResponse& res, const ServerConfig& config)
{
	const LocationConfig& location = config.matchLocation(req.getUri());
	const std::pair<int, std::string> redirect = location.getReturn();

	if (redirect.first)
	{
		Logger::instance().log(DEBUG, "Router::isRedirect -> " + redirect.second);
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
	const LocationConfig& location = config.matchLocation(req.getUri());
	std::string uploadPath = location.getUploadPath();
	const std::string& uri = req.getUri();

	Logger::instance().log(DEBUG, "Router::isUpload comparing uri=" + uri + " uploadPath=" + uploadPath);

	// Only POST or PUT
	if (req.getMethod() != RequestMethod::POST && req.getMethod() != RequestMethod::PUT)
		return (false);

	// Uploads disenable
	if (!location.getUploadEnabled())
	{
		Logger::instance().log(WARNING, "Router::isUpload disabled for this location (403)");
		res.setStatusCode(ResponseStatus::Forbidden);
		req.setRouteType(RouteType::Error);
		return (false);
	}

	// Normalize
	std::string basePath = uploadPath;
	if (basePath.empty())
		return (false);

	// Relative
	if (basePath[0] != '/')
		basePath = config.getRoot() + "/" + basePath;

	// Match
	if (uri.find(location.getPath()) == 0)
	{
		// if dir and writable
		struct stat s;
		if (stat(basePath.c_str(), &s) != 0 || !S_ISDIR(s.st_mode))
		{
			Logger::instance().log(WARNING, "Router::isUpload directory missing: " + basePath);
			res.setStatusCode(ResponseStatus::InternalServerError);
			req.setRouteType(RouteType::Error);
			return (false);
		}
		if (access(basePath.c_str(), W_OK) != 0)
		{
			Logger::instance().log(WARNING, "Router::isUpload path not writable: " + basePath);
			res.setStatusCode(ResponseStatus::Forbidden);
			req.setRouteType(RouteType::Error);
			return (false);
		}

		// OK
		req.setRouteType(RouteType::Upload);
		return (true);
	}

	return (false);
}


bool Router::isAutoIndex(const std::string& index, HttpRequest& req, const ServerConfig& config)
{
	const LocationConfig& location = config.matchLocation(req.getUri());

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


bool	Router::isStaticFile(const std::string& index, HttpRequest& req, HttpResponse& res)
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
			req.setResolvedPath(path);
			return (true);
		}

		Logger::instance().log(WARNING, "Router::isStaticFile forbidden access to " + path);
		res.setStatusCode(ResponseStatus::Forbidden);
	}
	return (false);
}

bool	Router::isCgi(const LocationConfig& loc, HttpRequest& req, HttpResponse& res)
{
	std::string cgiPath = loc.getCgiPath();

	if (!hasCgiExtension(loc, req.getResolvedPath()))
		return (false);

	struct stat s;

	if (stat(req.getResolvedPath().c_str(), &s) != 0 || !S_ISREG(s.st_mode))
		return (false);

	// if (cgiPath.empty())
	// {
	// 	Logger::instance().log(DEBUG, "Router::isCgi skipped: no CGI path configured");
	// 	return (false);
	// }

	if (req.getResolvedPath().find(cgiPath) == std::string::npos)
		return (false);

	if (access(req.getResolvedPath().c_str(), X_OK) != 0)
	{
		Logger::instance().log(WARNING, "Router::isCgi file not executable: " + req.getResolvedPath());
		res.setStatusCode(ResponseStatus::Forbidden);
		return (false);
	}

	Logger::instance().log(DEBUG, "Router::isCgi detected");

	return (true);
}

bool Router::hasCgiExtension(const LocationConfig& loc, const std::string& path)
{
	std::string ext = getFileExtension(path);
	const std::map<std::string, std::string>& cgiMap = loc.getCgiExtension();

	std::map<std::string, std::string>::const_iterator it = cgiMap.find(ext);
	if (it != cgiMap.end())
	{
		return (true);
	}
	return (false);
}

