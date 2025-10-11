#include "request/HttpRequest.hpp"

HttpRequest::HttpRequest()
{
	setMethod(RequestMethod::INVALID);
	setParseError(ResponseStatus::OK);
	getMeta().setContentLength(0);
	getMeta().setChunked(false);
	getMeta().setConnectionClose(false);
	getMeta().setExpectContinue(false);
	setRequestState(RequestState::RequestLine);
	setParsingChunkSize(true);
	setExpectingChunkSeparator(false);
	setCurrentChunkSize(0);
}

HttpRequest::~HttpRequest() {}

void	HttpRequest::setMethod(const RequestMethod::Method& method)
{
	this->_method = method;
}

void	HttpRequest::setUri(const std::string& uri) { this->_uri = uri; }

void	HttpRequest::setQueryString(const std::string queryString) { this->_queryString = queryString; }

void	HttpRequest::setMajor(int major) { this->_major = major; }

void	HttpRequest::setMinor(int minor) { this->_minor = minor; }

void	HttpRequest::addHeader(const std::string& name, const std::string& value)
{
	//refactor insertion
	std::map<std::string, std::string>::iterator it;
	it = this->_headers.find(name);
	if (it != _headers.end())
	{
		std::string current = it->second;
		std::string new_ = current + "," + value;
		_headers.erase(it);
		_headers[name] = new_;
	}
	else
		_headers[name] = value;
}

void	HttpRequest::appendBody(const std::string& body)
{
	this->_body += body;
}

void	HttpRequest::appendBody(char c)
{
	this->_body.push_back(c);
}

void	HttpRequest::setParseError(ResponseStatus::code reason)
{
	this->_parseError = reason;
}

void	HttpRequest::setRequestState(RequestState::state state)
{
	this->_state = state;
}

void	HttpRequest::setRouteType(RouteType::route route)
{
	this->_route = route;
}

void	HttpRequest::appendRaw(const std::string& chunk)
{
	this->_rawRequest += chunk;
}

void	HttpRequest::clearBuffer(void)
{
	this->_buffer.clear();
}

void	HttpRequest::clearChunkBuffer(void)
{
	this->_chunkBuffer.clear();
}

void	HttpRequest::setCurrentChunkSize(int size)
{
	this->_currentChunkSize = size;
}

void	HttpRequest::setParsingChunkSize(bool value)
{
	this->_parsingChunkSize = value;
}

void	HttpRequest::setExpectingChunkSeparator(bool value)
{
	this->_expectingChunkSeparator = value;
}

void	HttpRequest::setResolvedPath(const std::string path)
{
	this->_resolvedPath = path;
}

void	HttpRequest::reset(void)
{
	this->_method = RequestMethod::INVALID;
	this->_uri.clear();
	this->_major = 0;
	this->_minor = 0;
	this->_headers.clear();
	this->_meta.resetMeta();
	this->_body.clear();
	this->_parseError = ResponseStatus::OK;
	this->_state = RequestState::RequestLine;
	this->_route = RouteType::Error;
	this->_rawRequest.clear();
	this->_buffer.clear();
	this->_chunkBuffer.clear();
	this->_currentChunkSize = 0;
	this->_parsingChunkSize = false;
	this->_expectingChunkSeparator = false;
	this->_resolvedPath.clear();
}

RequestMethod::Method	HttpRequest::getMethod(void) const
{
	return (this->_method);
}

const std::string	HttpRequest::methodToString(void) const
{
	switch (this->_method)
	{
		case RequestMethod::GET:
			return "GET";
		case RequestMethod::HEAD:
			return "HEAD";
		case RequestMethod::POST:
			return "POST";
		case RequestMethod::PUT:
			return "PUT";
		case RequestMethod::DELETE:
			return "DELETE";
		case RequestMethod::TRACE:
			return "TRACE";
		case RequestMethod::CONNECT:
			return "CONNECT";
		case RequestMethod::PATCH:
			return "PATCH";
		case RequestMethod::INVALID:
			return "INVALID";
		default:
			return "INVALID";
	}
}

const std::string&	HttpRequest::getUri(void) const { return (this->_uri); }

const std::string&	HttpRequest::getQueryString(void) const { return (this->_queryString); }

const std::vector<int>	HttpRequest::getHttpVersion(void) const
{
	int version[2] = {this->_major, this->_minor};

	return (std::vector<int>(version, version + 2));
}

const std::string& HttpRequest::getHeader(const std::string& name) const
{
	std::map<std::string, std::string>::const_iterator c_it;

	c_it = this->_headers.find(name);
	if (c_it != this->_headers.end())
		return (c_it->second);
	return (name);
}

const std::map<std::string, std::string>&	HttpRequest::getAllHeaders(void) const
{
	return (this->_headers);
}

const RequestMeta&	HttpRequest::getMeta(void) const
{
	return (this->_meta);
}

RequestMeta&	HttpRequest::getMeta(void)
{
	return (this->_meta);
}
const std::string&	HttpRequest::getBody(void) const
{
	return (this->_body);
}

ResponseStatus::code	HttpRequest::getParseError(void) const
{
	return (this->_parseError);
}

RequestState::state	HttpRequest::getState(void) const
{
	return (this->_state);
}

RouteType::route	HttpRequest::getRouteType(void) const
{
	return (this->_route);
}

std::string&	HttpRequest::getRaw(void)
{
	return (this->_rawRequest);
}

std::string&	HttpRequest::getBuffer(void)
{
	return (this->_buffer);
}

std::string&	HttpRequest::getChunkBuffer(void)
{
	return (this->_chunkBuffer);
}

int	HttpRequest::getCurrentChunkSize(void) const
{
	return (this->_currentChunkSize);
}

bool	HttpRequest::isParsingChunkSize(void) const
{
	return (this->_parsingChunkSize);
}

bool	HttpRequest::isExpectingChunkSeparator(void) const
{
	return (this->_expectingChunkSeparator);
}

const std::string	HttpRequest::getResolvedPath(void) const
{
	return (this->_resolvedPath);
}

// int	HttpRequest::getClientFD(void) const
// {
// 	return (this->_clientFD);
// }
