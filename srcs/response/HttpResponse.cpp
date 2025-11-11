#include "response/HttpResponse.hpp"
#include <utils/Logger.hpp>

/**
 * @brief Default constructor for HttpResponse.
 *
 * Initializes response with default HTTP/1.1 OK status and no headers.
 */
HttpResponse::HttpResponse()
{
	setStatusCode(ResponseStatus::OK);
	setVersion("1.1");
	setChunked(false);
}

/**
 * @brief Destructor for HttpResponse.
 */
HttpResponse::~HttpResponse() {}

/**
 * @brief Sets the HTTP status code and updates the corresponding reason phrase.
 */
void	HttpResponse::setStatusCode(const ResponseStatus::code& code)
{
	this->_statusCode = code;
	setReasonPhrase(code);
}

/**
 * @brief Sets the textual reason phrase for a given status code.
 */
void	HttpResponse::setReasonPhrase(const ResponseStatus::code& code)
{
	this->_reasonPhrase = statusCodeToString(code);
}

/**
 * @brief Defines the HTTP version string (e.g., "1.1").
 */
void	HttpResponse::setVersion(const std::string& version)
{
	this->_version = version;
}

/**
 * @brief Appends text data to the response body.
 */
void	HttpResponse::appendBody(const std::string& body)
{
	this->_body += body;
}

/**
 * @brief Appends a single character to the response body.
 */
void	HttpResponse::appendBody(char c)
{
	this->_body.push_back(c);
}

/**
 * @brief Adds a header field to the HTTP response.
 *
 * If the header already exists, the values are concatenated using a comma.
 */
void	HttpResponse::addHeader(const std::string& name, const std::string& value)
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
 * @brief Sets whether the response will use chunked transfer encoding.
 */
void	HttpResponse::setChunked(bool chunked)
{
	this->_chunked = chunked;
}

/**
 * @brief Resets the response to its default, empty state.
 *
 * Clears headers, body, and resets HTTP metadata.
 */
void	HttpResponse::reset(void)
{
	this->_statusCode = ResponseStatus::OK;
	this->_reasonPhrase.clear();
	this->_version.clear();
	this->_headers.clear();
	this->_body.clear();
	this->_chunked = false;
	Logger::instance().log(DEBUG, "HttpResponse::reset complete");
}

/**
 * @brief Returns the current HTTP status code.
 */
ResponseStatus::code	HttpResponse::getStatusCode() const
{
	return (this->_statusCode);
}

/**
 * @brief Returns the reason phrase associated with the status code.
 */
const std::string&	HttpResponse::getReasonPhrase(void) const
{
	return (this->_reasonPhrase);
}

/**
 * @brief Returns the HTTP version of this response.
 */
const std::string&	HttpResponse::getHttpVersion(void) const
{
	return (this->_version);
}

/**
 * @brief Retrieves a header value by name.
 */
const std::string&	HttpResponse::getHeader(const std::string& name) const
{
	std::map<std::string, std::string>::const_iterator c_it;

	c_it = this->_headers.find(name);
	if (c_it != this->_headers.end())
		return (c_it->second);
	return (name);
}

/**
 * @brief Returns all response headers as a constant reference.
 */
const std::map<std::string, std::string>&	HttpResponse::getHeaders(void) const
{
	return (this->_headers);
}

/**
 * @brief Returns the current response body.
 */
const std::string&	HttpResponse::getBody(void) const { return (this->_body); }

/**
 * @brief Returns true if response uses chunked transfer encoding.
 */
bool	HttpResponse::isChunked(void) const { return (this->_chunked); }

/**
 * @brief Converts an HTTP status code to its standard reason phrase string.
 */
const std::string	HttpResponse::statusCodeToString(const ResponseStatus::code& code)
{
	switch (code)
	{
		// 1xx
		case ResponseStatus::Continue:
			return ("Continue");
		case ResponseStatus::SwitchingProtocols:
			return ("Switching Protocols");

		// 2xx
		case ResponseStatus::OK:
			return ("OK");
		case ResponseStatus::Created:
			return ("Created");
		case ResponseStatus::Accepted:
			return ("Accepted");
		case ResponseStatus::NoContent:
			return ("No Content");
		case ResponseStatus::PartialContent:
			return ("Partial Content");

		// 3xx
		case ResponseStatus::MovedPermanently:
			return ("Moved Permanently");
		case ResponseStatus::Found:
			return ("Found");
		case ResponseStatus::SeeOther:
			return ("See Other");
		case ResponseStatus::NotModified:
			return ("Not Modified");
		case ResponseStatus::TemporaryRedirect:
			return ("Temporary Redirect");

		// 4xx
		case ResponseStatus::BadRequest:
			return ("Bad Request");
		case ResponseStatus::Unauthorized:
			return ("Unauthorized");
		case ResponseStatus::Forbidden:
			return ("Forbidden");
		case ResponseStatus::NotFound:
			return ("Not Found");
		case ResponseStatus::MethodNotAllowed:
			return ("Method Not Allowed");
		case ResponseStatus::RequestTimeout:
			return ("Request Timeout");
		case ResponseStatus::Conflict:
			return ("Conflict");
		case ResponseStatus::Gone:
			return ("Gone");
		case ResponseStatus::PayloadTooLarge:
			return ("Payload Too Large");
		case ResponseStatus::UriTooLong:
			return ("URI Too Long");
		case ResponseStatus::UnsupportedMediaType:
			return ("Unsupported Media Type");
		case ResponseStatus::ExpectationFailed:
			return ("Expectation Failed");

		// 5xx
		case ResponseStatus::InternalServerError:
			return ("Internal Server Error");
		case ResponseStatus::NotImplemented:
			return ("Not Implemented");
		case ResponseStatus::BadGateway:
			return ("Bad Gateway");
		case ResponseStatus::ServiceUnavailable:
			return ("Service Unavailable");
		case ResponseStatus::GatewayTimeout:
			return ("Gateway Timeout");
		case ResponseStatus::HttpVersionNotSupported:
			return ("HTTP Version Not Supported");

		default:
			return ("Unknown Status");
	}
}
