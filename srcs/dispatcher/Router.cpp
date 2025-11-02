#include <string.h>
#include <algorithm>
#include <sys/stat.h>
#include <unistd.h>
#include <dispatcher/Router.hpp>
#include <response/ResponseStatus.hpp>
#include <config/ServerConfig.hpp>
#include <utils/Logger.hpp>

void	Router::computeResolvedPath(HttpRequest& req, const std::string& rawRoot)
{
	std::string resolvedPath;
	std::string uri  = req.getUri();

	std::string root = rawRoot;
	if (!root.empty() && root[root.size() - 1] == '/')
		root.erase(root.size() - 1);
	
	if (!uri.empty() && uri[0] == '/')
		resolvedPath = root + uri;
	else
		resolvedPath = root + "/" + uri;
	
	req.setResolvedPath(resolvedPath);
}

bool	Router::checkErrorStatus(ResponseStatus::code status, HttpRequest& req, HttpResponse& res)
{
	if (status != ResponseStatus::OK)
	{
		req.setRouteType(RouteType::Error);
		res.setStatusCode(status);
		return (true);
	}
	return (false);
}

void	Router::resolve(HttpRequest& req, HttpResponse& res, ServerConfig const& config)
{
	const LocationConfig& location = config.mathLocation(req.getUri());
	std::string index;
	std::string root;
	std::string cgiPath = location.getCgiPath();

	if (location.getHasIndexFiles())
		index = location.getIndex();

	if (location.getHasRoot())
		root = location.getRoot();
	else
		root = config.getRoot();

	ResponseStatus::code status = ResponseStatus::OK;

	if (req.getParseError() != ResponseStatus::OK)
	{
		req.setRouteType(RouteType::Error);
		res.setStatusCode(req.getParseError());
		return ;
	}

	computeResolvedPath(req, root);

	Logger::instance().log(DEBUG,
		"computeResolvedPath [Path -> " + req.getResolvedPath() + "]");
	
	if (isRedirect(req, res, config))
	{
		req.setRouteType(RouteType::Redirect);
		return ;
	}
	if (isAutoIndex(index, req, config))
	{
		req.setRouteType(RouteType::AutoIndex);
		return ;
	}
	if (checkErrorStatus(status, req, res))
		return ;
	if (isUpload(req, res, config))
	{
		req.setRouteType(RouteType::Upload);
		return ;
	}
	if (checkErrorStatus(status, req, res))
		return ;
	if (isStaticFile(index, status, req))
	{
		req.setRouteType(RouteType::StaticPage);
		if (req.getMethod() != RequestMethod::GET)
		{
			req.setRouteType(RouteType::Error);
			res.setStatusCode(ResponseStatus::MethodNotAllowed);
		}
		return ;
	}
	if (checkErrorStatus(status, req, res))
		return ;
	if (isCgi(cgiPath, req.getResolvedPath(), status))
	{
		req.setRouteType(RouteType::CGI);
		return ;
	}
	if (checkErrorStatus(status, req, res))
		return ;

	req.setRouteType(RouteType::Error);
	res.setStatusCode(ResponseStatus::NotFound);
}

bool	Router::isUpload(HttpRequest& req, HttpResponse& res, ServerConfig const& config)
{
	const LocationConfig& location = config.mathLocation(req.getUri());

	std::string uploadPath = location.getUploadPath();
	const std::string& uri = req.getUri();
	Logger::instance().log(DEBUG, "Router::isUpload compare URI -> " + uri
		+ " | upload path -> " + uploadPath);
	if (uri == uploadPath &&
		(req.getMethod() == RequestMethod::POST || req.getMethod() == RequestMethod::PUT))
	{
		if (!location.getUploadEnabled())
		{
			res.setStatusCode(ResponseStatus::MethodNotAllowed);
			return (false);
		}
		return (true);
	}
	return (false);
}

bool Router::isStaticFile(const std::string& index, ResponseStatus::code& status, HttpRequest& req)
{
	Logger::instance().log(DEBUG, "[Started] Router::isStaticFile");
	std::string _resolvedPath = req.getResolvedPath();

	struct stat s;

	if (stat(_resolvedPath.c_str(), &s) == 0 && S_ISDIR(s.st_mode))
	{
		Logger::instance().log(DEBUG, "Router::isStaticFile Its a directory");
		if (_resolvedPath[_resolvedPath.length() - 1] != '/')
		{
			_resolvedPath += '/';
		}
		_resolvedPath += index;
		Logger::instance().log(DEBUG, "Router::isStaticFile Its a directory, path -> " + _resolvedPath);
		if (stat(_resolvedPath.c_str(), &s) != 0)
			return (false);
	}

	if (stat(_resolvedPath.c_str(), &s) == 0 && S_ISREG(s.st_mode))
	{
		Logger::instance().log(DEBUG, "Router::isStaticFile does it exist");
		if (access(_resolvedPath.c_str(), R_OK) == 0)
		{
			req.setResolvedPath(_resolvedPath);
			if (hasCgiExtension(_resolvedPath))
				return (false);
			return (true);
		}
		else
		{
			status = ResponseStatus::Forbidden;
			return (false);
		}
	}
	else
		return (false);
}

bool	Router::isCgi(const std::string& cgiPath, const std::string resolvedPath, ResponseStatus::code& status)
{
	std::string _resolvedPath = resolvedPath;
	struct stat s;

	if (stat(_resolvedPath.c_str(), &s) != 0 || !S_ISREG(s.st_mode))
	{
		return (false);
	}
	if (access(_resolvedPath.c_str(), X_OK) != 0)
	{
		status = ResponseStatus::Forbidden;
		return (false);
	}
	if (_resolvedPath.find(cgiPath) != std::string::npos)
		return (true);

	if (hasCgiExtension(resolvedPath))
		return (true);
	return (false);
}

bool	Router::isAutoIndex(const std::string& index, HttpRequest& req, ServerConfig const& config)
{
	const LocationConfig& location = config.mathLocation(req.getUri());
	if (!location.getHasAutoIndex())
	{
		if (!config.getAutoindex())
			return (false);
	}
	std::string _resolvedPath = req.getResolvedPath();
	struct stat s;

	if (stat(_resolvedPath.c_str(), &s) == 0 && S_ISDIR(s.st_mode))
	{
		std::string indexPath = _resolvedPath;
		if (indexPath[indexPath.length() - 1] != '/')
			indexPath += '/';
		indexPath += index;
		
		struct stat indexStat;
		if (stat(indexPath.c_str(), &indexStat) != 0 || !S_ISREG(indexStat.st_mode))
			return (true);
	}
	return (false);
}

bool	Router::hasCgiExtension(const std::string& path)
{
	const std::string cgiExtensions[] = {".cgi", ".pl", ".py"};

	std::string::size_type dotPos = path.rfind('.');
	if (dotPos != std::string::npos)
	{
		std::string ext = path.substr(dotPos);
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
		for (size_t i = 0; i < sizeof(cgiExtensions)/sizeof(cgiExtensions[0]); ++i)
		{
			if (ext == cgiExtensions[i])
				return (true);
		}
	}
	return (false);
}

bool	Router::isRedirect(HttpRequest& req, HttpResponse& res, ServerConfig const& config)
{
	const LocationConfig& location = config.mathLocation(req.getUri());
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
