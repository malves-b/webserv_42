#include <fstream>
#include <sstream>
#include <response/ResponseBuilder.hpp>
#include <utils/Logger.hpp>
#include <utils/string_utils.hpp>
#include <utils/Signals.hpp>

/**
 * @brief Generates a timestamp string in RFC 1123 format for HTTP headers.
 *
 * Example output: "Tue, 11 Nov 2025 18:45:00 GMT"
 */
const std::string	ResponseBuilder::fmtTimestamp(void)
{
	std::time_t now = std::time(0);
	tm* timeinfo = std::gmtime(&now);
	char buf[100];
	std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", timeinfo);
	return (std::string(buf));
}

/**
 * @brief Adds mandatory HTTP headers to the response.
 *
 * Includes "Date" and "Server" headers.
 */
void	ResponseBuilder::setMinimumHeaders(HttpResponse& response)
{
	response.addHeader("Date", fmtTimestamp());
	response.addHeader("Server", "Webservinho/1.0");
}

/**
 * @brief Generates a simple HTML error page for a given HTTP status code.
 *
 * Returns a basic HTML document with a status code and an image from http.cat.
 */
std::string	ResponseBuilder::errorPageGenerator(ResponseStatus::code code)
{
	std::ostringstream oss;

	oss << "<!DOCTYPE html>\r\n"
		<< "<html>\r\n"
		<< "<head><title>Error " << code << "</title></head>\r\n"
		<< "<body style=\"text-align:center;padding:50px;\">\r\n"
		<< "<h1>" << code << " - Error</h1>\r\n"
		<< "<img src=\"https://http.cat/" << code
		<< "\" alt=\"Error HTTP " << code << "\" style=\"max-width:80%;height:auto;\">\r\n"
		<< "</body>\r\n"
		<< "</html>\r\n";

	return (oss.str());
}

/**
 * @brief Constructs the complete HTTP response string.
 *
 * Includes status line, headers, and body (unless chunked).
 */
const std::string	ResponseBuilder::responseWriter(HttpResponse& response)
{
	Logger::instance().log(DEBUG, "[Started] ResponseBuilder::responseWriter");

	std::ostringstream oss;

	// Status line
	oss << "HTTP/" << response.getHttpVersion() << " "
		<< response.getStatusCode() << " "
		<< response.getReasonPhrase() << "\r\n";

	// Headers
	const std::map<std::string, std::string>& headers = response.getHeaders();
	for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
		oss << it->first << ": " << it->second << "\r\n";

	oss << "\r\n";

	// Append body only if not chunked
	if (!response.isChunked())
		oss << response.getBody();

	oss << "\r\n";

	Logger::instance().log(DEBUG, "[Finished] ResponseBuilder::responseWriter");
	return (oss.str());
}

/**
 * @brief Prepares an HTTP response for a static page.
 *
 * Sets Content-Type, Content-Length, and appends the page content.
 */
void	ResponseBuilder::handleStaticPageOutput(HttpResponse& response,
	const std::string output, const std::string& mimeType)
{
	response.setChunked(false);
	response.addHeader("Content-Type", mimeType);
	response.addHeader("Content-Length", toString(output.size()));
	response.appendBody(output);
}

/**
 * @brief Processes CGI output and builds an HTTP response from it.
 *
 * Splits headers and body, parses the "Status" field if present, and fills
 * the HttpResponse object with the appropriate headers and body.
 */
void	ResponseBuilder::handleCgiOutput(HttpResponse& response, const std::string& output)
{
	std::size_t sep = output.find("\r\n\r\n");
	if (sep == std::string::npos)
	{
		Logger::instance().log(ERROR, "ResponseBuilder: invalid CGI output (no header separator)");
		response.setStatusCode(ResponseStatus::BadGateway);
		return ;
	}

	std::string headersPart = output.substr(0, sep);
	std::string bodyPart = output.substr(sep + 4);

	std::istringstream headerStream(headersPart);
	std::string line;

	while (std::getline(headerStream, line))
	{
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);

		std::size_t colon = line.find(':');
		if (colon == std::string::npos)
			continue ;

		std::string key = trim(line.substr(0, colon));
		std::string value = trim(line.substr(colon + 1));

		response.addHeader(key, value);

		if (toLower(key) == "status")
		{
			std::istringstream iss(value);
			int status;
			if (iss >> status)
				response.setStatusCode(static_cast<ResponseStatus::code>(status));
		}
	}

	response.appendBody(bodyPart);
	response.addHeader("Content-Length", toString(bodyPart.size()));
}

/**
 * @brief Assembles the complete HTTP response, including error handling.
 *
 * Adds default headers, manages persistent connections, and generates
 * custom or default error pages when needed.
 * @callgraph
 */
void	ResponseBuilder::build(ClientConnection& client, HttpRequest& req, HttpResponse& res)
{
	Logger::instance().log(DEBUG, "[Started] ResponseBuilder::build");
	Logger::instance().log(DEBUG,
		"ResponseBuilder: StatusCode -> " + toString(res.getStatusCode()));

	setMinimumHeaders(res);
	res.setReasonPhrase(res.getStatusCode());
	res.setVersion("1.1");
	res.addHeader("Connection", "keep-alive");

	// Handle explicit connection close
	if (req.getMeta().shouldClose())
	{
		res.addHeader("Connection", "close");
		req.getMeta().setConnectionClose(true);
	}

	// Generate error page if response >= 400
	if (res.getStatusCode() >= 400)
	{
		if (shouldCloseConnection(res.getStatusCode()))
		{
			res.addHeader("Connection", "close");
			req.getMeta().setConnectionClose(true);
		}

		if (!errorPageConfig(client.getServerConfig().getRoot(), res, client.getServerConfig()))
		{
			std::string content = errorPageGenerator(res.getStatusCode());
			handleStaticPageOutput(res, content, "text/html");
		}
	}

	Logger::instance().log(DEBUG, "[Finished] ResponseBuilder::build");
}

/**
 * @brief Attempts to load and serve a custom error page from configuration.
 *
 * If unavailable, returns false to trigger default error page generation.
 */
bool	ResponseBuilder::errorPageConfig(const std::string& root, HttpResponse& res, const ServerConfig& config)
{
	int statusCode = static_cast<int>(res.getStatusCode());
	const std::map<int, std::string>& errorsPages = config.getErrorPage();

	std::map<int, std::string>::const_iterator it = errorsPages.find(statusCode);
	if (it == errorsPages.end())
		return (false);

	std::string path = root + it->second;
	std::ifstream file(path.c_str(), std::ios::binary);

	if (!file)
	{
		Logger::instance().log(ERROR, "ResponseBuilder: cannot open error page file -> " + path);
		res.setStatusCode(ResponseStatus::InternalServerError);
		return (false);
	}

	std::ostringstream buffer;
	buffer << file.rdbuf();
	file.close();

	handleStaticPageOutput(res, buffer.str(), "text/html");

	Logger::instance().log(DEBUG, "ResponseBuilder: served custom error page -> " + path);
	return (true);
}

/**
 * @brief Determines whether the connection should be closed based on status code.
 */
bool	ResponseBuilder::shouldCloseConnection(int statusCode)
{
	switch (statusCode)
	{
		case 400: case 408: case 411: case 413:
		case 414: case 431: case 500: case 501: case 505:
			return (true);
		default:
			return (false);
	}
}
