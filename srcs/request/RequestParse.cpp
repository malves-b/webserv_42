#include <iostream>
#include <exception>
#include <cstdlib>
#include <vector>
#include <utils/string_utils.hpp>
#include <utils/Logger.hpp>
#include <response/ResponseStatus.hpp>
#include <request/RequestParse.hpp>
#include <request/RequestMethod.hpp>
#include <config/ServerConfig.hpp>
#include <config/LocationConfig.hpp>

void	RequestParse::handleRawRequest(const std::string& chunk, HttpRequest& req, const ServerConfig& config)
{
	req.appendRaw(chunk);
	std::string& rawRequest = req.getRaw();
	std::string& buffer = req.getBuffer();
	std::size_t i = 0;

	if (req.getState() == RequestState::Complete)
		return ;

	while (i < rawRequest.size() && req.getState() != RequestState::Complete)
	{
		if (req.getState() < RequestState::Body)
		{
			char ch = rawRequest[i];

			if (ch == '\r')
			{
				if (i + 1 >= rawRequest.size())
					break ;

				if (rawRequest[i + 1] != '\n')
				{
					req.setParseError(ResponseStatus::BadRequest);
					return ;
				}

				if (req.getState() == RequestState::RequestLine)
				{
					if (!buffer.empty())
						requestLine(buffer, req, config);

					if (req.getParseError() != ResponseStatus::OK)
					{
						req.setRequestState(RequestState::Complete);
						return ;
					}

					req.setRequestState(RequestState::Headers);
				}
				else
				{
					if (buffer.empty())
					{
						if (req.getMeta().getContentLength() == 0 && !req.getMeta().isChunked())
							req.setRequestState(RequestState::Complete);
						else
							req.setRequestState(RequestState::Body);

						req.clearBuffer();
						i += 2;
						continue ;
					}
					else
					{
						headers(buffer, req, config.getClientMaxBodySize());
					}
				}
				req.clearBuffer();
				i += 2;
				continue ;
			}
			buffer.push_back(ch);
			++i;
			continue ;
		}

		if (req.getState() == RequestState::Body)
		{
			body(rawRequest[i], req, config.getClientMaxBodySize());
			++i;
		}
	}

	if (i > 0)
		req.getRaw().erase(0, i);

	if (req.getMeta().isChunked() && req.getState() == RequestState::Complete)
	{
		Logger::instance().log(DEBUG, "RequestParse: finalizing chunked body for CGI");
		req.getMeta().setChunked(false);
		req.getMeta().setContentLength(req.getBody().size());
		req.addHeader("content-length", toString(req.getBody().size()));

		// Remove Transfer-Encoding
		if (req.hasHeader("transfer-encoding"))
			req.removeHeader("transfer-encoding");
	}

	Logger::instance().log(DEBUG,
		"RequestParse::handleRawRequest consumed=" + toString(i) + " remaining=" + toString(rawRequest.size()));
}

void RequestParse::requestLine(const std::string& buffer, HttpRequest& req, const ServerConfig& config)
{
	Logger::instance().log(DEBUG, "[Started] RequestParse::reqLine");

	const std::vector<std::string> tokens = split(buffer, " ");

	if (tokens.size() != 3)
	{
		req.setParseError(ResponseStatus::BadRequest);
		return ;
	}

	uri(tokens[1], req);
	method(tokens[0], req, config);

	const std::string version = tokens[2];

	if (version.size() < 8 || version.substr(0, 5) != "HTTP/" || version.find('.') == std::string::npos)
	{
		req.setParseError(ResponseStatus::BadRequest);
		return ;
	}

	std::vector<std::string> parts = split(version.substr(5), ".");
	if (parts.size() != 2)
	{
		req.setParseError(ResponseStatus::BadRequest);
		return ;
	}

	if (parts[0] != "1" || parts[1] != "1")
	{
		req.setParseError(ResponseStatus::HttpVersionNotSupported);
		return ;
	}

	req.setMajor(std::atoi(parts[0].c_str()));
	req.setMinor(std::atoi(parts[1].c_str()));

	Logger::instance().log(DEBUG, "[Finished] RequestParse::reqLine");
}

void	RequestParse::method(const std::string& method, HttpRequest& req, const ServerConfig& config)
{
	if (method == "GET")
		req.setMethod(RequestMethod::GET);
	else if (method == "POST")
		req.setMethod(RequestMethod::POST);
	else if (method == "DELETE")
		req.setMethod(RequestMethod::DELETE);
	else if (method == "PUT")
		req.setMethod(RequestMethod::PUT);
	else
	{
		req.setMethod(RequestMethod::INVALID);
		req.setParseError(ResponseStatus::MethodNotAllowed);
	}

	checkMethod(req, config);
}

void	RequestParse::uri(const std::string str, HttpRequest& req)
{
	std::string uri = str;
	std::string::size_type queryPos = uri.find('?');

	if (queryPos != std::string::npos)
		uri = str.substr(0, queryPos);

	if (uri.length() > MAX_URI)
	{
		Logger::instance().log(ERROR, "RequestParse::uri URI too long");
		req.setParseError(ResponseStatus::UriTooLong);
		req.setRequestState(RequestState::Complete);
		return ;
	}

	req.setUri(uri);
	req.setQueryString(extractQueryString(str));
}

void	RequestParse::headers(const std::string& buffer, HttpRequest& req, std::size_t maxBodySize)
{
	Logger::instance().log(DEBUG, "[Started] RequestParse::headers");

	std::string::size_type pos = buffer.find(":");

	if (pos == std::string::npos)
	{
		req.setParseError(ResponseStatus::BadRequest);
		req.setRequestState(RequestState::Complete);
		Logger::instance().log(ERROR, "RequestParse::headers BadRequest (missing colon)");
		return ;
	}

	std::string key = toLower(trim(buffer.substr(0, pos)));
	std::string value = trim((pos + 1 < buffer.size()) ? buffer.substr(pos + 1) : std::string());

	if (buffer.length() > MAX_HEADER_LINE)
	{
		req.setParseError(ResponseStatus::PayloadTooLarge);
		req.setRequestState(RequestState::Complete);
		Logger::instance().log(ERROR, "RequestParse::headers PayloadTooLarge");
		return ;
	}

	if (key == "host")
		req.getMeta().setHost(value);
	else if (key == "content-length")
	{
		long size = std::atol(value.c_str());

		if (isGreaterThanMaxBodySize(size, maxBodySize))
		{
			req.setParseError(ResponseStatus::PayloadTooLarge);
			req.setRequestState(RequestState::Complete);
			Logger::instance().log(ERROR, "RequestParse::headers Content-Length exceeds limit");
			return ;
		}
		req.getMeta().setContentLength(size);
	}
	else if (key == "transfer-encoding")
	{
		std::string v = toLower(value);
		if (v.find("chunked") != std::string::npos)
			req.getMeta().setChunked(true);
		else if (v != "identity")
		{
			req.setParseError(ResponseStatus::BadRequest);
			req.setRequestState(RequestState::Complete);
			Logger::instance().log(ERROR, "RequestParse::headers Unsupported transfer-encoding: " + value);
			return ;
		}
	}
	else if (key == "connection")
		req.getMeta().setConnectionClose(toLower(value) == "close");
	else if (key == "expect")
	{
		if (toLower(value) == "100-continue")
		{
			req.getMeta().setExpectContinue(true);
			Logger::instance().log(DEBUG, "RequestParse::headers Expect: 100-continue");
		}
		else
		{
			req.setParseError(ResponseStatus::BadRequest);
			req.setRequestState(RequestState::Complete);
			Logger::instance().log(ERROR, "RequestParse::headers BadRequest on Expect header");
			return ;
		}
	}

	req.addHeader(key, value);
	Logger::instance().log(DEBUG, "[Finished] RequestParse::headers");
}

void	RequestParse::body(char c, HttpRequest& req, std::size_t maxBodySize)
{
	if (!req.getMeta().isChunked())
	{
		if ((req.getBody().size() & 0x3FFF) == 0) // log every ~16KB
		{
			Logger::instance().log(DEBUG,
				"Body progress: " + toString(req.getBody().size()) + "/" +
				toString(req.getMeta().getContentLength()));
		}

		req.appendBody(c);

		if (req.getBody().size() >= req.getMeta().getContentLength())
			req.setRequestState(RequestState::Complete);
	}
	else
	{
		bodyChunked(c, req, maxBodySize);
	}
}

void	RequestParse::bodyChunked(char c, HttpRequest& req, std::size_t maxBodySize)
{
	std::string& buffer = req.getBuffer();
	std::string& chunkBuffer = req.getChunkBuffer();

	if (isGreaterThanMaxBodySize(req.getCurrentChunkSize(), maxBodySize))
	{
		req.setParseError(ResponseStatus::PayloadTooLarge);
		req.setRequestState(RequestState::Complete);
		return ;
	}

	if (req.isExpectingChunkSeparator())
	{
		buffer.push_back(c);

		if (buffer.size() >= 2 &&
			buffer[buffer.size() - 2] == '\r' &&
			buffer[buffer.size() - 1] == '\n')
		{
			req.clearBuffer();
			req.setExpectingChunkSeparator(false);
			req.setParsingChunkSize(true);
		}
		else if (buffer.size() > 2)
		{
			req.setParseError(ResponseStatus::BadRequest);
			req.setRequestState(RequestState::Complete);
		}
		return ;
	}

	if (req.isParsingChunkSize())
	{
		buffer.push_back(c);

		if (buffer.size() >= 2 &&
			buffer[buffer.size() - 2] == '\r' &&
			buffer[buffer.size() - 1] == '\n')
		{
			int size = stringToHex(buffer.substr(0, buffer.size() - 2));
			req.clearBuffer();

			if (size < 0)
			{
				req.setParseError(ResponseStatus::BadRequest);
				req.setRequestState(RequestState::Complete);
				return ;
			}
			if (size == 0)
			{
				req.setRequestState(RequestState::Complete);
				return ;
			}
			req.setCurrentChunkSize(size);
			req.setParsingChunkSize(false);
		}
		return ;
	}

	chunkBuffer.push_back(c);

	if ((int)chunkBuffer.size() == req.getCurrentChunkSize())
	{
		req.appendBody(chunkBuffer);
		req.clearChunkBuffer();
		req.setExpectingChunkSeparator(true);
	}
}

std::string	RequestParse::extractQueryString(const std::string uri)
{
	std::string::size_type queryPos = uri.find('?');

	if (queryPos != std::string::npos)
		return (uri.substr(queryPos + 1));

	return ("");
}

bool	RequestParse::isGreaterThanMaxBodySize(std::size_t size, std::size_t maxBodySize)
{
	if (size > maxBodySize)
		return (true);

	return (false);
}

void	RequestParse::checkMethod(HttpRequest& req, const ServerConfig& config)
{
	if (req.getMethod() == RequestMethod::INVALID)
		return ;

	const LocationConfig& location = config.matchLocation(req.getUri());
	std::vector<RequestMethod::Method> methods = location.getMethods();
	RequestMethod::Method m = req.getMethod();

	for (size_t i = 0; i < methods.size(); ++i)
	{
		if (methods[i] == m)
			return ;
	}

	req.setParseError(ResponseStatus::MethodNotAllowed);
}
