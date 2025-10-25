#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <dispatcher/StaticPageHandler.hpp>
#include <response/ResponseBuilder.hpp>
#include <utils/string_utils.hpp>
#include <utils/Logger.hpp>

const std::string StaticPageHandler::detectMimeType(const std::string& resolvedPath)
{
	// Posição do último ponto
	std::string::size_type dotPos = resolvedPath.rfind('.');
	if (dotPos == std::string::npos)
		return ("application/octet-stream");

	std::string ext = toLower(resolvedPath.substr(dotPos + 1));

	// tabela de MIME types
	static std::map<std::string, std::string> mimeTypes;
	if (mimeTypes.empty())
	{
		mimeTypes["html"] = "text/html";
		mimeTypes["htm"]  = "text/html";
		mimeTypes["css"]  = "text/css";
		mimeTypes["js"]   = "application/javascript";
		mimeTypes["json"] = "application/json";
		mimeTypes["png"]  = "image/png";
		mimeTypes["jpg"]  = "image/jpeg";
		mimeTypes["jpeg"] = "image/jpeg";
		mimeTypes["gif"]  = "image/gif";
		mimeTypes["svg"]  = "image/svg+xml";
		mimeTypes["txt"]  = "text/plain";
		mimeTypes["pdf"]  = "application/pdf";
	}

	std::map<std::string, std::string>::const_iterator it = mimeTypes.find(ext);
	if (it != mimeTypes.end())
		return (it->second);

	// fallback
	return ("application/octet-stream");
}


void	StaticPageHandler::handle(HttpRequest& req, HttpResponse& res)
{
	Logger::instance().log(DEBUG, "[Started] StaticPageHandler::handle");
	Logger::instance().log(DEBUG,
			"StaticPageHandler::handle [Path -> " + req.getResolvedPath() + "]");
	struct stat st;
	if (stat(req.getResolvedPath().c_str(), &st) != 0)
	{
		res.setStatusCode(ResponseStatus::NotFound);
		return ;
	}

	std::ifstream file(req.getResolvedPath().c_str(), std::ios::binary);

	if (!file)
	{
		res.setStatusCode(ResponseStatus::InternalServerError);
		return ;
	}
	std::ostringstream buffer;
	buffer << file.rdbuf();

	ResponseBuilder::handleStaticPageOutput(res,
		buffer.str(), detectMimeType(req.getResolvedPath()));
	Logger::instance().log(DEBUG, "[Finished] StaticPageHandler::handle");
}
