#include <sstream>
#include <sstream>
#include <response/ResponseBuilder.hpp>
#include <utils/Logger.hpp>
#include <utils/string_utils.hpp>
#include <init/ServerConfig.hpp>

const std::string	ResponseBuilder::fmtTimestamp(void)
{
	std::string timestamp;
	std::time_t now = std::time(0);
	tm* timeinfo = std::gmtime(&now);
	char buf[100];
	std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", timeinfo);
	timestamp += buf;
	return (timestamp);
}

void	ResponseBuilder::setMinimumHeaders(HttpResponse& response)
{
	response.addHeader("Date", fmtTimestamp());
	response.addHeader("Server", "Webservinho/1.0");
}

std::string	ResponseBuilder::errorPageGenerator(ResponseStatus::code code)
{
	std::ostringstream oss;

	oss << "<!DOCTYPE html>\r\n"
		<< "<html>\r\n"
		<< "<head><title>Error " << code << "</title></head>\r\n"
		<< "<body style=\"text-align: center; padding: 50px;\">\r\n"
		<< "<h1>" << code << " - Error</h1>\r\n"
		<< "<img src=\"https://http.cat/" << code
		<< "\" alt=\"Error HTTP " << code << "\" style=\"max-width: 80%; height: auto;\">\r\n"
		<< "</body>\r\n"
		<< "</html>\r\n";

	return (oss.str());
}

const std::string	ResponseBuilder::responseWriter(HttpResponse& response)
{
	std::ostringstream oss;

	Logger::instance().log(DEBUG, "[Started] ResponseBuilder::responseWriter");

	//response line
	oss << "HTTP/" << response.getHttpVersion() << " "
		<< response.getStatusCode() << " "
		<< response.getReasonPhrase() << "\r\n";

	//headers
	const std::map<std::string, std::string>& headers = response.getHeaders();
	std::map<std::string, std::string>::const_iterator it;
	for (it = headers.begin(); it != headers.end(); ++it)
	{
		oss << it->first << ":" << it->second << "\r\n";
	}

	oss << "\r\n";

	if (!response.isChunked())
		oss << response.getBody();
	//else
		//TODO body chunks
	oss << "\r\n";
	Logger::instance().log(DEBUG, "[Finished] ResponseBuilder::responseWriter");
	return (oss.str());
}

void	ResponseBuilder::handleStaticPageOutput(HttpResponse& response,
			const std::string output, const std::string& mimeType)
{
	response.setChunked(false);
	response.addHeader("Content-Type", mimeType);
	response.addHeader("Content-Length", toString(output.size()));
	response.appendBody(output);
}

void	ResponseBuilder::handleCgiOutput(HttpResponse& response, const std::string& output)
{
	std::size_t sep = output.find("\r\n\r\n");
	if (sep == std::string::npos)
	{
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
			line = line.substr(0, line.size() - 1);

		std::size_t colon = line.find(":");
		if (colon != std::string::npos)
		{
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
	}

	response.appendBody(bodyPart);
	response.addHeader("Content-Length", toString(bodyPart.size()));
}

void	ResponseBuilder::build(HttpRequest& req, HttpResponse& res)
{
	Logger::instance().log(DEBUG, "[Started] ResponseBuilder::build");
	Logger::instance().log(DEBUG,
		"[ResponseBuilder::build] [StatusCode ->" + toString(res.getStatusCode()) + "]");

	setMinimumHeaders(res);
	res.addHeader("connection", "keep-live");

	if (req.getMeta().shouldClose())
	{
		res.addHeader("connection", "close");
		req.getMeta().setConnectionClose(true);
	}
	if (res.getStatusCode() >= 400)
	{
		if (shouldCloseConnection(res.getStatusCode()))
		{
			res.addHeader("connection", "close");
			req.getMeta().setConnectionClose(true);
		}
		if (!errorPageConfig(res))
		{
			std::string content = errorPageGenerator(res.getStatusCode());
			handleStaticPageOutput(res, content, "text/html");
		}
	}
	Logger::instance().log(DEBUG, "[Finished] ResponseBuilder::build");
}

bool	ResponseBuilder::errorPageConfig(HttpResponse& res)
{
	int statusCode = static_cast<int>(res.getStatusCode());
	const std::string path = ServerConfig::instance().errorPage(statusCode);
	if (path != "")
	{
		std::ifstream file(path.c_str(), std::ios::binary);

		if (!file)
		{
			res.setStatusCode(ResponseStatus::InternalServerError);
			return (false);
		}
		std::ostringstream buffer;
		buffer << file.rdbuf();
		handleStaticPageOutput(res,	buffer.str(), "text/html");
		return (true);
	}
	return (false);
}

bool	ResponseBuilder::shouldCloseConnection(int statusCode)
{
	switch (statusCode)
	{
		case 400: // Bad Request
		case 408: // Request Timeout
		case 411: // Length Required
		case 413: // Payload Too Large
		case 414: // URI Too Long
		case 431: // Request Header Fields Too Large
		case 500: // Internal Server Error
		case 501: // Not Implemented
		case 505: // HTTP Version Not Supported
			return (true);
		default:
			return (false);
	}
}
