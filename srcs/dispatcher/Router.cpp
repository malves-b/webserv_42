#include <string.h>
#include <algorithm>
#include <sys/stat.h>
#include <unistd.h>
#include <dispatcher/Router.hpp>
#include <response/ResponseStatus.hpp>
#include <init/ServerConfig.hpp>
#include <utils/Logger.hpp>

void	Router::computeResolvedPath(HttpRequest& request)
{
	std::string resolvedPath;
	std::string uri  = request.getUri();

	std::string root = ServerConfig::instance().root;
	if (!root.empty() && root[root.size() - 1] == '/')
		root.erase(root.size() - 1);
	
	if (!uri.empty() && uri[0] == '/')
		resolvedPath = root + uri;
	else
		resolvedPath = root + "/" + uri;
	
	request.setResolvedPath(resolvedPath);
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

void	Router::resolve(HttpRequest& request, HttpResponse& response)
{

	std::string index = ServerConfig::instance().index;
	std::string cgiPath = ServerConfig::instance().cgiPath;
	ResponseStatus::code status = ResponseStatus::OK;

	if (request.getParseError() != ResponseStatus::OK)
	{
		request.setRouteType(RouteType::Error);
		response.setStatusCode(request.getParseError());
		return ;
	}

	computeResolvedPath(request);

	Logger::instance().log(DEBUG,
		"computeResolvedPath [Path -> " + request.getResolvedPath() + "]");
	
	if (isRedirect(request, response))
	{
		request.setRouteType(RouteType::Redirect);
		return ;
	}

	if (isAutoIndex(index, request))
	{
		request.setRouteType(RouteType::AutoIndex);
		return;
	}
	if (isUpload("/upload", request)) //TODO config
	{
		request.setRouteType(RouteType::Upload);
		return ;
	}
	if (isStaticFile(index, status, request))
	{
		request.setRouteType(RouteType::StaticPage);
		if (request.getMethod() != RequestMethod::GET) //Config ?
		{
			//TODO function
			request.setRouteType(RouteType::Error);
			response.setStatusCode(ResponseStatus::MethodNotAllowed);
		}
		return ;
	}
	if (checkErrorStatus(status, request, response))
		return ;
	if (isCgi(cgiPath, request.getResolvedPath(), status))
	{
		request.setRouteType(RouteType::CGI);
		return ;
	}
	if (checkErrorStatus(status, request, response))
		return ;

	request.setRouteType(RouteType::Error);
	response.setStatusCode(ResponseStatus::NotFound);
}

bool	Router::isUpload(const std::string& uploadPath, HttpRequest& req)
{
	const std::string& uri = req.getUri();
	Logger::instance().log(DEBUG, "Router::isUpload compare URI -> " + uri
		+ " | upload path -> " + uploadPath);
	if (uri == uploadPath &&
		(req.getMethod() == RequestMethod::POST || req.getMethod() == RequestMethod::PUT) /* &&
		!location.upload_path.empty() */)
	{
		return true;
	}
	return false;
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

/* */
bool Router::isAutoIndex(const std::string& index, HttpRequest& req)
{
    std::string _resolvedPath = req.getResolvedPath();
    struct stat s;

    if (stat(_resolvedPath.c_str(), &s) == 0 && S_ISDIR(s.st_mode))
    {
        if (ServerConfig::instance().autoindex)
        {
            std::string indexPath = _resolvedPath;
            if (indexPath[indexPath.length() - 1] != '/')
                indexPath += '/';
            indexPath += index;
            
            struct stat indexStat;
            if (stat(indexPath.c_str(), &indexStat) != 0 || !S_ISREG(indexStat.st_mode)){
                return true;}
        }
    }
    return false;
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

bool	Router::isRedirect(HttpRequest& req, HttpResponse& res)
{
	//TODO config complete
	//for (vector redirect struct?)
	Logger::instance().log(DEBUG, "Router::isRedirect -> " + req.getUri());
	if (req.getUri() == "/static/contact.html")
	{
		req.getMeta().setRedirect(true);
		res.setChunked(false);
		res.addHeader("Location", ServerConfig::instance().redirect);
		res.setStatusCode(ResponseStatus::TemporaryRedirect); //TODO config
		return (true);
	}
	return (false);
}
