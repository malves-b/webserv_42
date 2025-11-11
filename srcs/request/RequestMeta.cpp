#include "request/RequestMeta.hpp"

/**
 * @brief Default constructor for RequestMeta.
 *
 * Initializes an empty metadata container for an HTTP request.
 */
RequestMeta::RequestMeta() {}

/**
 * @brief Destructor for RequestMeta.
 */
RequestMeta::~RequestMeta() {}

/**
 * @brief Sets the Content-Length value for the request.
 */
void	RequestMeta::setContentLength(std::size_t content_length)
{
	this->_contentLength = content_length;
}

/**
 * @brief Defines whether the request uses chunked transfer encoding.
 */
void	RequestMeta::setChunked(bool chunked)
{
	this->_chunked = chunked;
}

/**
 * @brief Defines whether the connection should be closed after the response.
 */
void	RequestMeta::setConnectionClose(bool connection_close)
{
	this->_connectionClose = connection_close;
}

/**
 * @brief Sets the Expect: 100-continue flag for the request.
 */
void	RequestMeta::setExpectContinue(bool expect_continue)
{
	this->_expectContinue = expect_continue;
}

/**
 * @brief Sets whether this request is associated with a redirect.
 */
void	RequestMeta::setRedirect(bool redirect)
{
	this->_isRedirect = redirect;
}

/**
 * @brief Stores the value of the Host header for this request.
 */
void	RequestMeta::setHost(const std::string& host)
{
	this->_host = host;
}

/**
 * @brief Resets all metadata fields to default values.
 *
 * Clears flags and stored headers for reuse of the same object.
 */
void	RequestMeta::resetMeta(void)
{
	this->_contentLength = 0;
	this->_chunked = false;
	this->_connectionClose = false;
	this->_expectContinue = false;
	this->_isRedirect = false;
	this->_host.clear();
}

/**
 * @brief Returns the Content-Length value.
 */
std::size_t	RequestMeta::getContentLength(void) const { return (this->_contentLength); }

/**
 * @brief Returns true if transfer encoding is chunked.
 */
bool	RequestMeta::isChunked(void) const { return (this->_chunked); }

/**
 * @brief Returns true if the connection should be closed.
 */
bool	RequestMeta::shouldClose(void) const { return (this->_connectionClose); }

/**
 * @brief Returns true if the client expects a 100-Continue response.
 */
bool	RequestMeta::getExpectContinue(void) const { return (this->_expectContinue); }

/**
 * @brief Returns true if this request corresponds to an HTTP redirect.
 */
bool	RequestMeta::isRedirect(void) const { return (this->_isRedirect); }

/**
 * @brief Returns the stored Host header value.
 */
std::string	RequestMeta::getHost(void) const { return (this->_host); }
