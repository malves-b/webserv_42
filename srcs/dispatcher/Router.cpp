#include <cstring>
#include <algorithm>
#include <sys/stat.h>
#include <unistd.h>
#include <dispatcher/Router.hpp>
#include <response/ResponseStatus.hpp>
#include <config/ServerConfig.hpp>
#include <utils/Logger.hpp>
#include <utils/string_utils.hpp>

/**
 * @brief Main entry point for determining how an HTTP request should be handled.
 *
 * The Router decides the appropriate route type based on request attributes,
 * location configuration, and server rules. It checks for:
 * - Parser errors or invalid URIs
 * - Path traversal attempts
 * - Redirects
 * - CGI execution
 * - File uploads
 * - Autoindex directories
 * - Static file serving
 * - DELETE operations
 *
 * On success, sets the correct RouteType and HTTP response status.
 */
void	Router::resolve(HttpRequest& req, HttpResponse& res, const ServerConfig& config)
{
	// Step 1: Handle parser-level errors before routing logic
	if (req.getParseError() != ResponseStatus::OK)
	{
		Logger::instance().log(WARNING, "Router: Request parse error detected");
		req.setRouteType(RouteType::Error);
		res.setStatusCode(req.getParseError());
		return ;
	}

	// Step 2: Match the configuration block ("location") for the requested URI
	const LocationConfig& loc = config.matchLocation(req.getUri());
	const std::string index = loc.getHasIndexFiles() ? loc.getIndex() : "";
	const std::string root  = loc.getHasRoot() ? loc.getRoot() : config.getRoot();
	const std::string cgiPath = loc.getCgiPath();

	// Step 3: Path traversal security check
	if (hasParentTraversal(req.getUri()))
	{
		Logger::instance().log(WARNING, "Router: Path traversal attempt blocked: " + req.getUri());
		req.setRouteType(RouteType::Error);
		res.setStatusCode(ResponseStatus::Forbidden);
		return ;
	}

	// Step 4: Build filesystem path corresponding to request URI
	computeResolvedPath(req, loc, config);

	// Step 5: Handle configured HTTP redirects
	if (isRedirect(req, res, config))
	{
		Logger::instance().log(INFO, "Router: Route type = Redirect");
		req.setRouteType(RouteType::Redirect);
		return ;
	}

	// Step 6: Handle CGI execution requests
	if (isCgi(loc, req, res))
	{
		Logger::instance().log(INFO, "Router: Route type = CGI");
		req.setRouteType(RouteType::CGI);
		return ;
	}

	if (checkErrorStatus(req, res))
		return ;

	// Step 7: Handle file uploads (POST/PUT)
	if (isUpload(req, res, config))
	{
		Logger::instance().log(INFO, "Router: Route type = Upload");
		req.setRouteType(RouteType::Upload);
		return ;
	}
	if (checkErrorStatus(req, res))
		return ;

	// Step 8: Handle AutoIndex directory listings
	if (index.empty() && isAutoIndex(index, req, config))
	{
		Logger::instance().log(INFO, "Router: Route type = AutoIndex");
		req.setRouteType(RouteType::AutoIndex);
		return ;
	}

	// Step 9: Handle static files (GET requests)
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

	// Step 10: Handle DELETE requests explicitly
	if (req.getMethod() == RequestMethod::DELETE)
	{
		req.setRouteType(RouteType::Delete);
		return ;
	}

	// Step 11: Default case: route not found (404)
	Logger::instance().log(WARNING, "Router: No matching route found (404)");
	res.setStatusCode(ResponseStatus::NotFound);
	req.setRouteType(RouteType::Error);
}

/**
 * @brief Computes the absolute filesystem path of the requested resource.
 *
 * Combines the URI with the root or CGI path, removes location prefixes,
 * and appends index files when appropriate.
 */
void	Router::computeResolvedPath(HttpRequest& req,
								const LocationConfig& location,
								const ServerConfig& config)
{
	std::string uri = req.getUri();
	std::string root;

	// Determine which root path to use (CGI, location, or server)
	if (!location.getCgiPath().empty())
		root = location.getCgiPath();
	else if (location.getHasRoot())
		root = location.getRoot();
	else
		root = config.getRoot();

	// Remove trailing slash from location path
	std::string locPath = location.getPath();
	if (locPath.size() > 1 && locPath[locPath.size() - 1] == '/')
		locPath.erase(locPath.size() - 1);

	// Compute relative path
	std::string relativePath = uri;
	if (uri.find(locPath) == 0)
		relativePath = uri.substr(locPath.length());
	if (!relativePath.empty() && relativePath[0] == '/')
		relativePath.erase(0, 1);

	std::string resolved = joinPaths(root, relativePath);

	// If path is a directory and has index files, append them
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

/**
 * @brief Checks if the response already contains an error status.
 *
 * @return true if an error code is set, otherwise false.
 */
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

/**
 * @brief Checks if the request triggers an HTTP redirect.
 *
 * Reads `return` directive from location config and updates response headers.
 */
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

/**
 * @brief Determines if the current request is a valid upload operation.
 *
 * Checks request method (POST/PUT), upload enablement, and directory write access.
 */
bool	Router::isUpload(HttpRequest& req, HttpResponse& res, const ServerConfig& config)
{
	const LocationConfig& location = config.matchLocation(req.getUri());
	std::string uploadPath = location.getUploadPath();
	const std::string& uri = req.getUri();

	Logger::instance().log(DEBUG, "Router::isUpload comparing uri=" + uri + " uploadPath=" + uploadPath);

	// Only POST or PUT methods are valid for upload
	if (req.getMethod() != RequestMethod::POST && req.getMethod() != RequestMethod::PUT)
		return (false);

	// Uploads disabled at location level
	if (!location.getUploadEnabled())
	{
		Logger::instance().log(WARNING, "Router::isUpload disabled for this location (403)");
		return (false);
	}

	std::string basePath = uploadPath;
	if (basePath.empty())
		return (false);

	// Handle relative upload paths
	if (basePath[0] != '/')
		basePath = config.getRoot() + "/" + basePath;

	// Validate directory existence and write access
	if (uri.find(location.getPath()) == 0)
	{
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

		// Directory OK
		req.setRouteType(RouteType::Upload);
		return (true);
	}

	return (false);
}

/**
 * @brief Determines whether to enable AutoIndex for a directory listing.
 */
bool	Router::isAutoIndex(const std::string& index, HttpRequest& req, const ServerConfig& config)
{
	const LocationConfig& location = config.matchLocation(req.getUri());

	bool autoIndexEnabled = config.getAutoindex();

	// Location-specific directive overrides global setting
	if (location.getHasAutoIndex())
		autoIndexEnabled = location.getAutoindex();

	if (!autoIndexEnabled)
		return (false);

	std::string path = req.getResolvedPath();
	struct stat s;

	// AutoIndex only applies to directories without index files
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

/**
 * @brief Checks if the resolved path corresponds to a static file.
 *
 * Handles both direct file and directory-with-index cases.
 */
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

/**
 * @brief Determines if the request targets a valid CGI script.
 *
 * Checks configured CGI path, file extension, and execution permissions.
 */
bool	Router::isCgi(const LocationConfig& loc, HttpRequest& req, HttpResponse& res)
{
	std::string cgiPath = loc.getCgiPath();

	if (!hasCgiExtension(loc, req.getResolvedPath()))
		return (false);

	struct stat s;

	if (stat(req.getResolvedPath().c_str(), &s) != 0 || !S_ISREG(s.st_mode))
		return (false);

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

/**
 * @brief Checks whether a file extension matches a configured CGI mapping.
 */
bool	Router::hasCgiExtension(const LocationConfig& loc, const std::string& path)
{
	std::string ext = getFileExtension(path);
	const std::map<std::string, std::string>& cgiMap = loc.getCgiExtension();

	std::map<std::string, std::string>::const_iterator it = cgiMap.find(ext);
	return (it != cgiMap.end());
}
