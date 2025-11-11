#include "request/HttpRequest.hpp"
#include <utils/Logger.hpp>
#include <utils/string_utils.hpp>

/**
 * @brief Constructs a new HttpRequest with default initialized state.
 *
 * Sets default values for parsing, chunked transfer, and route type.
 */
HttpRequest::HttpRequest()
{
	setMethod(RequestMethod::INVALID);
	setParseError(ResponseStatus::OK);
	getMeta().setContentLength(0);
	getMeta().setChunked(false);
	getMeta().setConnectionClose(false);
	getMeta().setExpectContinue(false);
	getMeta().setRedirect(false);
	setRequestState(RequestState::RequestLine);
	setParsingChunkSize(true);
	setExpectingChunkSeparator(false);
	setCurrentChunkSize(0);
}

HttpRequest::~HttpRequest() {}

/**
 * @brief Sets the HTTP request method.
 */
void	HttpRequest::setMethod(const RequestMethod::Method& method)
{
	this->_method = method;
}

/**
 * @brief Sets the request target URI.
 */
void	HttpRequest::setUri(const std::string& uri) { this->_uri = uri; }

/**
 * @brief Sets the query string extracted from the URI.
 */
void	HttpRequest::setQueryString(const std::string queryString) { this->_queryString = queryString; }

/**
 * @brief Sets the HTTP major version number.
 */
void	HttpRequest::setMajor(int major) { this->_major = major; }

/**
 * @brief Sets the HTTP minor version number.
 */
void	HttpRequest::setMinor(int minor) { this->_minor = minor; }

/**
 * @brief Adds or appends a header field to the internal map.
 *
 * If the header already exists, values are concatenated with commas.
 */
void	HttpRequest::addHeader(const std::string& name, const std::string& value)
{
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

/**
 * @brief Appends data to the body of the request.
 */
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

/**
 * @brief Appends raw data to the internal request buffer.
 */
void	HttpRequest::appendRaw(const std::string& chunk)
{
	this->_rawRequest += chunk;
}

/**
 * @brief Clears the general parsing buffer.
 */
void	HttpRequest::clearBuffer(void)
{
	this->_buffer.clear();
}

/**
 * @brief Clears the temporary buffer used for chunked transfer decoding.
 */
void	HttpRequest::clearChunkBuffer(void)
{
	this->_chunkBuffer.clear();
}

/**
 * @brief Updates the size of the current chunk being parsed.
 */
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

/**
 * @brief Sets the absolute filesystem path resolved from the URI.
 */
void	HttpRequest::setResolvedPath(const std::string path)
{
	this->_resolvedPath = path;
}

/**
 * @brief Resets the HttpRequest object to its initial state.
 *
 * Clears headers, body, buffers, and resets parsing flags.
 */
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
	Logger::instance().log(DEBUG, "HttpRequest::reset complete");
}

/**
 * @brief Returns the current HTTP method.
 */
RequestMethod::Method	HttpRequest::getMethod(void) const
{
	return (this->_method);
}

/**
 * @brief Converts the HTTP method enum to its string representation.
 */
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

/**
 * @brief Returns the HTTP version as a pair of integers (major, minor).
 */
const std::vector<int>	HttpRequest::getHttpVersion(void) const
{
	int version[2] = {this->_major, this->_minor};
	return (std::vector<int>(version, version + 2));
}

/**
 * @brief Retrieves a header value by name (case-insensitive).
 */
const std::string&	HttpRequest::getHeader(const std::string& name) const
{
	std::map<std::string, std::string>::const_iterator c_it;

	std::string norm_name = toLower(name);
	c_it = this->_headers.find(norm_name);
	if (c_it != this->_headers.end())
		return (c_it->second);
	return (name);
}

/**
 * @brief Returns a const reference to the full headers map.
 */
const std::map<std::string, std::string>&	HttpRequest::getAllHeaders(void) const
{
	return (this->_headers);
}

/**
 * @brief Returns a const reference to the RequestMeta structure.
 */
const RequestMeta&	HttpRequest::getMeta(void) const
{
	return (this->_meta);
}

RequestMeta&	HttpRequest::getMeta(void)
{
	return (this->_meta);
}

/**
 * @brief Returns the body of the request.
 */
const std::string&	HttpRequest::getBody(void) const
{
	return (this->_body);
}

/**
 * @brief Returns the HTTP parse error code (if any).
 */
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

/**
 * @brief Returns the raw request buffer (used during parsing).
 */
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

/**
 * @brief Returns the resolved filesystem path corresponding to the URI.
 */
const std::string	HttpRequest::getResolvedPath(void) const
{
	return (this->_resolvedPath);
}

/**
 * @brief Checks whether a header with the given key exists.
 */
bool	HttpRequest::hasHeader(const std::string& key) const
{
	std::string lowerKey = toLower(key);
	return (_headers.find(lowerKey) != _headers.end());
}

/**
 * @brief Removes a header from the request by name.
 */
void	HttpRequest::removeHeader(const std::string& key)
{
	std::string lowerKey = toLower(key);
	_headers.erase(lowerKey);
}
