#include <iostream>
#include <exception>
#include <cstdlib>
#include <vector>
#include <request/RequestParse.hpp>
#include <request/RequestMethod.hpp>
#include <utils/string_utils.hpp>
#include <utils/Logger.hpp>
#include <response/ResponseStatus.hpp>
#include <init/ServerConfig.hpp>
#include <utils/Logger.hpp>

void	RequestParse::handleRawRequest(const std::string& chunk, HttpRequest& request)
{
	request.appendRaw(chunk);
	std::string& rawRequest = request.getRaw();

	std::string& buffer = request.getBuffer();
	std::size_t i = 0;

	if (request.getState() == RequestState::Complete)
		return ;

	while (i < rawRequest.size() && request.getState() != RequestState::Complete)
	{
		if (request.getState() < RequestState::Body)
		{
			char ch = rawRequest[i];
			if (ch == '\r')
			{
				if (i + 1 >= rawRequest.size())
					break ;
				if (rawRequest[i + 1] != '\n')
				{
					request.setParseError(ResponseStatus::BadRequest);
					return ;
				}
				if (request.getState() == RequestState::RequestLine)
				{
					if (!buffer.empty())
						requestLine(buffer, request);
					if (request.getParseError() != ResponseStatus::OK)
					{
						request.setRequestState(RequestState::Complete);
						return ;
					}
					request.setRequestState(RequestState::Headers);
				}
				else
				{
					if (buffer.empty())
					{
						if (request.getMeta().getContentLength() == 0 && !request.getMeta().isChunked())
							request.setRequestState(RequestState::Complete);
						else
							request.setRequestState(RequestState::Body);
						request.clearBuffer();
						i += 2;
						break ;
					}
					else
					{
						headers(buffer, request);
					}
				}
				request.clearBuffer();
				i += 2;
				continue ;
			}
			buffer.push_back(ch);
			++i;
			continue ;
		}
		if (request.getState() == RequestState::Body)
		{
			body(rawRequest[i], request);
			++i;
		}
	}
	if (i > 0)
		request.getRaw().erase(0, i);
	Logger::instance().log(DEBUG,
		"Consumed i=" + toString(i) + " of raw=" + toString(rawRequest.size()));
}

void	RequestParse::requestLine(const std::string& buffer, HttpRequest& request)
{
	Logger::instance().log(DEBUG, "[Started] RequestParse::requestLine");
	const std::vector<std::string> tokens = split(buffer, " ");

	if (tokens.size() != 3)
	{
		request.setParseError(ResponseStatus::BadRequest);
		return ;
	}

	method(tokens[0], request);
	uri(tokens[1], request);

	const std::string version = tokens[2];
	if (version.size() < 8 || version.substr(0, 5) != "HTTP/"
		|| version.find('.') == std::string::npos)
	{
		request.setParseError(ResponseStatus::BadRequest);
		return ;
	}

	std::vector<std::string> parts = split(version.substr(5), ".");
	if (parts.size() != 2)
	{
		request.setParseError(ResponseStatus::BadRequest);
		return ;
	}

	if (parts[0] != "1" || parts[1] != "1")
	{
		request.setParseError(ResponseStatus::HttpVersionNotSupported);
		return ;
	}

	request.setMajor(std::atoi(parts[0].c_str()));
	request.setMinor(std::atoi(parts[1].c_str()));
	Logger::instance().log(DEBUG, "[Finished] RequestParse::requestLine");
}

void	RequestParse::method(const std::string& method, HttpRequest& request)
{
	//TODO config
	if (method == "GET")
		request.setMethod(RequestMethod::GET);
	else if (method == "POST")
		request.setMethod(RequestMethod::POST);
	else if (method == "DELETE")
		request.setMethod(RequestMethod::DELETE);
	else if (method == "PUT")
		request.setMethod(RequestMethod::PUT);
	else
	{
		request.setMethod(RequestMethod::INVALID);
		request.setParseError(ResponseStatus::MethodNotAllowed);
	}
	checkMethod(request);
}

void	RequestParse::uri(const std::string str, HttpRequest& request)
{
	std::string uri = str;
	std::string::size_type queryPos = uri.find('?');
	if (queryPos != std::string::npos)
		uri = str.substr(0, queryPos);
	if (uri.length() > MAX_URI)
	{
		Logger::instance().log(ERROR, "RequestParse::uri Uri too long");
		request.setParseError(ResponseStatus::UriTooLong);
		request.setRequestState(RequestState::Complete);
		return ;
	}
	request.setUri(uri);
	request.setQueryString(extractQueryString(str));
}

void	RequestParse::headers(const std::string& buffer, HttpRequest& request)
{
	Logger::instance().log(DEBUG, "[Started] RequestParse::headers");
	std::string::size_type pos = buffer.find(":");

	if (pos == std::string::npos)
	{
		request.setParseError(ResponseStatus::BadRequest);
		request.setRequestState(RequestState::Complete);
		Logger::instance().log(ERROR, "RequestParse::headers BadRequest");
		return ;
	}

	std::string key = toLower(trim(buffer.substr(0, pos)));
	std::string value = trim((pos + 1 < buffer.size()
							? buffer.substr(pos + 1) : std::string()));
	
	if (buffer.length() > MAX_HEADER_LINE)
	{
		request.setParseError(ResponseStatus::PayloadTooLarge);
		request.setRequestState(RequestState::Complete);
		Logger::instance().log(ERROR, "RequestParse::headers PayloadTooLarge");
		return ;
	}

	if (key == "host")
		request.getMeta().setHost(value);
	else if (key == "content-length")
	{
		long size = std::atol(value.c_str());
		if (isGreaterThanMaxBodySize(size))
		{
			request.setParseError(ResponseStatus::PayloadTooLarge);
			request.setRequestState(RequestState::Complete);
			Logger::instance().log(ERROR, "RequestParse::headers PayloadTooLarge");
			return ;
		}
		request.getMeta().setContentLength(size);
	}
	else if (key == "transfer-encoding")
		request.getMeta().setChunked(true);
	else if (key == "connection")
		request.getMeta().setConnectionClose(toLower(value) == "close");
	else if (key == "expect")
	{
		if (toLower(value) == "100-continue")
		{
			request.getMeta().setExpectContinue(true);
			Logger::instance().log(DEBUG, "RequestParse::headers -> EXPECT 100");
		}
		else
		{
			request.setParseError(ResponseStatus::BadRequest);
			request.setRequestState(RequestState::Complete);
			Logger::instance().log(ERROR, "RequestParse::headers BadRequest");
			return ;
		}
	}
	request.addHeader(key, value);
	Logger::instance().log(DEBUG, "[Finished] RequestParse::headers");
}

void	RequestParse::body(char c, HttpRequest& request)
{
	if (!request.getMeta().isChunked())
	{
		if ((request.getBody().size() & 0x3FFF) == 0) { // a cada ~16KB
			Logger::instance().log(DEBUG,
				"Body progress: " + toString(request.getBody().size()) + "/" +
				toString(request.getMeta().getContentLength()));
		}
		request.appendBody(c);
		if (request.getBody().size() >= request.getMeta().getContentLength())
			request.setRequestState(RequestState::Complete);

	}
	else
	{
		bodyChunked(c, request);
	}
}

void	RequestParse::bodyChunked(char c, HttpRequest& request)
{
	std::string& buffer = request.getBuffer();
	std::string& chunkBuffer = request.getChunkBuffer();

	if (isGreaterThanMaxBodySize(request.getCurrentChunkSize()))
	{
		request.setParseError(ResponseStatus::PayloadTooLarge);
		request.setRequestState(RequestState::Complete);
		return ;
	}
	if (request.isExpectingChunkSeparator())
	{
		buffer.push_back(c);
		if (buffer.size() >= 2 &&
			buffer[buffer.size() - 2] == '\r' &&
			buffer[buffer.size() - 1] == '\n')
		{
			request.clearBuffer();
			request.setExpectingChunkSeparator(false);
			request.setParsingChunkSize(true);
		}
		else if (buffer.size() > 2)
		{
			request.setParseError(ResponseStatus::BadRequest);
			request.setRequestState(RequestState::Complete);
		}
		return ;
	}
	if (request.isParsingChunkSize())
	{
		buffer.push_back(c);

		if (buffer.size() >= 2 &&
			buffer[buffer.size() - 2] == '\r' &&
			buffer[buffer.size() - 1] == '\n')
		{

			int size = stringToHex(buffer.substr(0, buffer.size() - 2));
			request.clearBuffer();
			if (size < 0)
			{
				request.setParseError(ResponseStatus::BadRequest);
				request.setRequestState(RequestState::Complete);
				return ;
			}
			if (size == 0)
			{
				request.setRequestState(RequestState::Complete);
				return ;
			}
			request.setCurrentChunkSize(size);
			request.setParsingChunkSize(false);
		}
		return ;
	}

	chunkBuffer.push_back(c);

	if ( (int) chunkBuffer.size() == request.getCurrentChunkSize())
	{
		request.appendBody(chunkBuffer);
		request.clearChunkBuffer();
		request.setExpectingChunkSeparator(true);
	}
}

std::string	RequestParse::extractQueryString(const std::string uri)
{
	std::string::size_type query_pos = uri.find('?');
	if (query_pos != std::string::npos)
	{
		return uri.substr(query_pos + 1);
		uri.substr(0, query_pos);
	}
	else
	{
		return "";
	}
}

bool	RequestParse::isGreaterThanMaxBodySize(std::size_t size)
{
	if (size > ServerConfig::instance().client_max_body_size)
		return (true);
	return (false);
}

void	RequestParse::checkMethod(HttpRequest& request)
{
	if (request.getMethod() == RequestMethod::INVALID)
		return ;
	size_t size = ServerConfig::instance().allow_methods.size();
	RequestMethod::Method m = request.getMethod();
	for (size_t i = 0; i < size; ++i)
	{
		// Logger::instance().log(DEBUG, "RequestParse::checkMethod -> "
		// 		+ toString(ServerConfig::instance().allow_methods[i]));
		if (ServerConfig::instance().allow_methods[i] == m)
			return ;
	}
	request.setParseError(ResponseStatus::MethodNotAllowed);
	// Logger::instance().log(DEBUG, "RequestParse::checkMethod -> "
	// 	+ toString(ServerConfig::instance().allow_methods[i])
	// 	+ "!=" + toString(m));
}