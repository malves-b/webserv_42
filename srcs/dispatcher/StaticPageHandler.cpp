#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <dispatcher/StaticPageHandler.hpp>
#include <response/ResponseBuilder.hpp>
#include <utils/string_utils.hpp>
#include <utils/Logger.hpp>

/**
 * @brief Determines the MIME type based on the file extension.
 *
 * Used when serving static files. Falls back to "application/octet-stream"
 * if the extension is unknown.
 *
 * @param resolvedPath Full filesystem path of the requested file.
 * @return MIME type string (e.g., "text/html", "image/png").
 */
const std::string	StaticPageHandler::detectMimeType(const std::string& resolvedPath)
{
	std::string::size_type dotPos = resolvedPath.rfind('.');

	if (dotPos == std::string::npos)
		return ("application/octet-stream");

	std::string ext = toLower(resolvedPath.substr(dotPos + 1));

	// Lazy initialization: the MIME map is filled only once.
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

	// Default MIME type for unknown extensions
	return ("application/octet-stream");
}

/**
 * @brief Handles serving static files from disk.
 *
 * Checks if the requested file exists and is accessible, then reads it
 * into memory and writes it to the HTTP response body. Detects MIME type
 * automatically based on file extension.
 *
 * Error conditions:
 * - File not found → 404 Not Found
 * - Unable to open file → 500 Internal Server Error
 *
 * On success, delegates response generation to ResponseBuilder.
 * @callgraph
 */
void	StaticPageHandler::handle(HttpRequest& req, HttpResponse& res)
{
	Logger::instance().log(DEBUG, "[Started] StaticPageHandler::handle");
	Logger::instance().log(DEBUG,
		"StaticPageHandler: Requested path -> " + req.getResolvedPath());

	struct stat st;

	// Step 1: Verify file existence on disk
	if (stat(req.getResolvedPath().c_str(), &st) != 0)
	{
		Logger::instance().log(WARNING, "StaticPageHandler: File not found -> " + req.getResolvedPath());
		res.setStatusCode(ResponseStatus::NotFound);
		return ;
	}

	// Step 2: Attempt to open the file in binary mode
	std::ifstream file(req.getResolvedPath().c_str(), std::ios::binary);
	if (!file.is_open())
	{
		Logger::instance().log(ERROR, "StaticPageHandler: Failed to open file -> " + req.getResolvedPath());
		res.setStatusCode(ResponseStatus::InternalServerError);
		return ;
	}

	// Step 3: Read entire file into memory buffer
	std::ostringstream buffer;
	buffer << file.rdbuf();
	file.close();

	// Step 4: Determine MIME type
	const std::string& mime = detectMimeType(req.getResolvedPath());
	Logger::instance().log(DEBUG, "StaticPageHandler: MIME type detected -> " + mime);

	// Step 5: Build final HTTP response
	ResponseBuilder::handleStaticPageOutput(res, buffer.str(), mime);

	Logger::instance().log(DEBUG, "[Finished] StaticPageHandler::handle");
}
