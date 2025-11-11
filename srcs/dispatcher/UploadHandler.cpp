#include <fstream>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>
#include <dispatcher/UploadHandler.hpp>
#include <response/ResponseStatus.hpp>
#include <config/ServerConfig.hpp>
#include <utils/Logger.hpp>
#include <utils/string_utils.hpp>

/**
 * @brief Entry point for handling file uploads.
 *
 * Validates Content-Type, checks configuration, and processes multipart/form-data
 * requests to extract uploaded files. Each file part is saved to disk.
 */
void UploadHandler::handle(HttpRequest& request, HttpResponse& response, std::string uploadPath, const std::string& rootPath)
{
	Logger::instance().log(DEBUG, "[Started] UploadHandler::handle");
	Logger::instance().log(DEBUG, "UploadHandler: Content-Type raw=[" + request.getHeader("Content-Type") + "]");

	const std::string contentType = request.getHeader("Content-Type");

	// Ensure that upload path is defined in configuration.
	if (uploadPath.empty())
	{
		Logger::instance().log(ERROR, "UploadHandler: upload path not configured");
		response.setStatusCode(ResponseStatus::InternalServerError);
		return ;
	}

	// Normalize Content-Type to lowercase for comparison.
	std::string ctLower = contentType;
	std::transform(ctLower.begin(), ctLower.end(), ctLower.begin(), ::tolower);

	// Validate multipart form
	if (ctLower.find("multipart/form-data") == std::string::npos)
	{
		Logger::instance().log(ERROR, "UploadHandler: invalid Content-Type");
		response.setStatusCode(ResponseStatus::BadRequest);
		return ;
	}

	// Parse the multipart body (saves files internally)
	if (!parseMultipart(request.getBody(), contentType, uploadPath, rootPath))
	{
		Logger::instance().log(ERROR, "UploadHandler: failed to parse multipart body");
		response.setStatusCode(ResponseStatus::BadRequest);
		return ;
	}

	// On success, send a simple HTML confirmation
	response.setStatusCode(ResponseStatus::Created);
	response.addHeader("Content-Type", "text/html; charset=utf-8");
	response.appendBody("<html><body><h1>Upload successful!</h1></body></html>");
	response.addHeader("Content-Length", toString(response.getBody().size()));

	Logger::instance().log(DEBUG, "[Finished] UploadHandler::handle");
}

/**
 * @brief Extracts the multipart boundary string from the Content-Type header.
 *
 * Example header:
 *   Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryxyz
 *
 * @return Boundary string without surrounding quotes, or empty if not found.
 */
std::string UploadHandler::extractBoundary(const std::string& contentType)
{
	std::string ctLower = contentType;
	std::transform(ctLower.begin(), ctLower.end(), ctLower.begin(), ::tolower);

	const std::string key = "boundary=";
	size_t pos = ctLower.find(key);
	if (pos == std::string::npos)
		return ("");

	std::string value = trim_copy(contentType.substr(pos + key.length()));
	size_t semicolon = value.find(';');
	if (semicolon != std::string::npos)
		value = trim_copy(value.substr(0, semicolon));

	// Remove surrounding quotes if present
	if (value.size() >= 2)
	{
		if ((value[0] == '"' && value[value.size() - 1] == '"')
			|| (value[0] == '\'' && value[value.size() - 1] == '\''))
			value = value.substr(1, value.size() - 2);
	}

	return (value);
}

/**
 * @brief Parses the full multipart body into individual parts.
 *
 * Each part is separated by a boundary line. For each section,
 * the function extracts headers and payload, and delegates to parsePart().
 *
 * @return true if parsing succeeded, false otherwise.
 */
bool UploadHandler::parseMultipart(const std::string& body,
	const std::string& contentType, const std::string& uploadPath, const std::string& rootPath)
{
	std::string boundaryValue = extractBoundary(contentType);
	if (boundaryValue.empty())
		return (false);

	std::string delimiter = "--" + boundaryValue;
	std::string closingDelim = delimiter + "--";

	// Ensure body starts with the expected delimiter
	if (body.compare(0, delimiter.size(), delimiter) != 0)
		return (false);

	size_t pos = delimiter.size();
	if (pos + 2 <= body.size() && body[pos] == '\r' && body[pos + 1] == '\n')
		pos += 2;

	while (pos < body.size())
	{
		size_t next = body.find("\r\n" + delimiter, pos);
		if (next == std::string::npos)
			break ;

		// Extract a single part section
		std::string part = body.substr(pos, next - pos);
		if (!parsePart(part, uploadPath, rootPath))
			return (false);

		pos = next + 2 + delimiter.size();

		// Detect end of multipart stream
		if (body.compare(pos, 2, "--") == 0)
			return (true);

		// Skip CRLF before next boundary
		if (pos + 2 <= body.size() && body[pos] == '\r' && body[pos + 1] == '\n')
			pos += 2;
	}

	return (true);
}

/**
 * @brief Parses an individual multipart section.
 *
 * Extracts headers, locates filename (if any), and passes data to saveFile().
 *
 * @return true if the part was parsed successfully, false on error.
 */
bool UploadHandler::parsePart(const std::string& part, const std::string& uploadPath, const std::string& rootPath)
{
	const std::string sep = "\r\n\r\n";
	size_t hEnd = part.find(sep);
	if (hEnd == std::string::npos)
		return (false);

	std::string headers = part.substr(0, hEnd);
	std::string data = part.substr(hEnd + sep.size());

	// Remove trailing CRLF from data section
	if (data.size() >= 2 && data[data.size() - 2] == '\r' && data[data.size() - 1] == '\n')
		data.resize(data.size() - 2);

	std::istringstream hs(headers);
	std::string line;
	std::string filename;

	// Parse multipart headers to find "filename="
	while (std::getline(hs, line))
	{
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);

		std::string lowerLine = line;
		std::transform(lowerLine.begin(), lowerLine.end(), lowerLine.begin(), ::tolower);

		if (lowerLine.find("content-disposition:") == 0)
		{
			size_t fn = lowerLine.find("filename=");
			if (fn != std::string::npos)
			{
				std::string val = line.substr(fn + 9);
				val = trim_copy(val);

				// Strip quotes
				if (!val.empty() && (val[0] == '"' || val[0] == '\''))
				{
					char q = val[0];
					size_t endq = val.find(q, 1);
					if (endq != std::string::npos)
						val = val.substr(1, endq - 1);
					else
						val = val.substr(1);
				}

				// Extract only the filename (ignore full path)
				size_t slash = val.find_last_of("/\\");
				if (slash != std::string::npos)
					val = val.substr(slash + 1);

				filename = val;
			}
		}
	}

	// Skip parts without filename (e.g., form fields)
	if (filename.empty())
		return (true);

	saveFile(filename, uploadPath, data, rootPath);
	return (true);
}

/**
 * @brief Saves a parsed file part to disk.
 *
 * Ensures correct base path resolution and writes data in binary mode.
 * Logs errors for permission issues or missing directories.
 */
void UploadHandler::saveFile(const std::string& filename,
	const std::string& uploadPath, const std::string& data, const std::string& rootPath)
{
	std::string base = uploadPath;

	// Handle relative upload paths (joined to server root)
	if (base.size() > 0 && base[0] != '/')
		base = rootPath + "/" + base;

	std::string path = base + "/" + filename;

	Logger::instance().log(DEBUG, "UploadHandler: resolved path -> " + path);

	std::ofstream out(path.c_str(), std::ios::binary);
	if (!out.is_open())
	{
		Logger::instance().log(ERROR, "UploadHandler: cannot open file for writing: " + path);
		return;
	}

	out.write(data.c_str(), data.size());
	out.close();

	Logger::instance().log(DEBUG, "UploadHandler: saved file -> " + path);
}
