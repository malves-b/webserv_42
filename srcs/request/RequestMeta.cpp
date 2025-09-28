#include "request/RequestMeta.hpp"

RequestMeta::RequestMeta() {}

RequestMeta::~RequestMeta() {}

void	RequestMeta::setContentLength(std::size_t content_length)
{
	this->_contentLength = content_length;
}

void	RequestMeta::setChunked(bool chunked)
{
	this->_chunked = chunked;
}

void	RequestMeta::setConnectionClose(bool connection_close)
{
	this->_connectionClose = connection_close;
}

void	RequestMeta::setExpectContinue(bool expect_continue)
{
	this->_expectContinue = expect_continue;
}

void	RequestMeta::setHost(const std::string& host)
{
	this->_host = host;
}

void	RequestMeta::resetMeta(void)
{
	this->_contentLength = 0;
	this->_chunked = false;
	this->_connectionClose = false;
	this->_expectContinue = true;
	this->_host.clear();
}

std::size_t	RequestMeta::getContentLength(void) const { return (this->_contentLength); }

bool	RequestMeta::isChunked(void) const { return (this->_chunked); }

bool	RequestMeta::shouldClose(void) const { return (this->_connectionClose); }

bool	RequestMeta::getExpectContinue(void) const { return (this->_expectContinue); }

std::string	RequestMeta::getHost(void) const { return (this->_host); }