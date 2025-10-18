#include <init/ClientConnection.hpp>
#include <request/RequestParse.hpp>
#include <response/ResponseBuilder.hpp> // response
#include <utils/string_utils.hpp>
#include <utils/Logger.hpp>
#include <unistd.h> //close()
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring> //strerror()
#include <stdexcept>
#include <string>

ClientConnection::ClientConnection(int fd) : _fd(fd), _sentBytes(0), _keepAlive(true) {}

ClientConnection::ClientConnection() : _fd(-1), _sentBytes(0), _keepAlive(true) {}

//ClientConnection::ClientConnection(ClientConnection const& src) : _fd(src._fd) {}

ClientConnection::ClientConnection(ClientConnection const& src) : _fd(-1), _sentBytes(0), _keepAlive(src._keepAlive) {}

ClientConnection::~ClientConnection(void)
{
	// if (this->_fd != -1)
	// 	::close(this->_fd);
	if (_fd >= 0)
		::close(this->_fd);
}

void ClientConnection::adoptFD(int fd)
{
	if (_fd >= 0)
		::close(this->_fd);
	_fd = fd;
}

ssize_t	ClientConnection::recvData(void)
{
	if (_fd == -1)
	{
		throw std::runtime_error("error: recvData: fd == -1");
	}

	char	buffer[4096];
	ssize_t	bytesRecv;

	bytesRecv = ::recv(this->_fd, buffer, sizeof(buffer), 0);
	Logger::instance().log(DEBUG, "ClientConnection::recvData bytesRecv -> " + toString(bytesRecv));
	if (bytesRecv > 0)
	{
		_requestBuffer.append(buffer, bytesRecv); //If the received data has embedded nulls (unlikely in HTTP headers but possible in POST bodies), you’ll not truncate this way
		Logger::instance().log(DEBUG,
			"ClientConnection::recvData appended " + toString(bytesRecv) +
			" bytes, buffer total = " + toString(_requestBuffer.size()));
		RequestParse::handleRawRequest(_requestBuffer, _httpRequest);
		Logger::instance().log(DEBUG, "ClientConnection::recvData request -> " + _requestBuffer);
		_requestBuffer.clear();
		return (bytesRecv);
	}
	if (bytesRecv == 0)
		return (0);
	if (errno == EAGAIN || errno == EWOULDBLOCK)
		return (-1);
	return (-2);
	// std::string	errorMsg(strerror(errno));
	// throw std::runtime_error("error: recv: " + errorMsg);
}

ssize_t	ClientConnection::sendData(ClientConnection &client, size_t sent, size_t toSend)
{
	if (_fd == -1)
	{
		throw std::runtime_error("error: recvData: fd == -1");
	}

	ssize_t	bytesSent;

	const std::string& resp = client.getResponseBuffer();
	const char* p = resp.c_str() + sent;

	//MSG_NOSIGNAL to avoid SIGPIPE
	bytesSent = send(client.getFD(), p, toSend, MSG_NOSIGNAL);
	if (bytesSent >= 0)
		return (bytesSent);

	if (errno == EAGAIN || errno == EWOULDBLOCK)
		return (-1);
	return (-2);

	// std::string	errorMsg(strerror(errno));
	// throw	std::runtime_error("error: send: " + errorMsg);
}

bool	ClientConnection::completedRequest(void)
{
	if (_httpRequest.getState() == RequestState::Complete)
	{
		Logger::instance().log(DEBUG, "ClientConnection::completedRequest -> TRUE");
		Logger::instance().log(DEBUG,
			"Request completed ParseError ->" + toString(_httpRequest.getParseError()));
		return (true);
	}
	Logger::instance().log(DEBUG, "ClientConnection::completedRequest -> FALSE");
	Logger::instance().log(DEBUG, "ClientConnection::completedRequest State -> " +
		toString(_httpRequest.getState()));
	return (false);

	// if (_requestBuffer.find("\r\n\r\n") != std::string::npos)
	// 	return (true);
	// return (false);
}

void	ClientConnection::clearBuffer(void) //rename
{
	this->_requestBuffer.clear();
	this->_responseBuffer.clear();
}

int const&	ClientConnection::getFD(void) const
{
	return (this->_fd);
}

size_t const&	ClientConnection::getSentBytes(void) const
{
	return (this->_sentBytes);
}

std::string const&	ClientConnection::getRequestBuffer(void) const
{
	return (_requestBuffer);
}

std::string const&	ClientConnection::getResponseBuffer(void) const
{
	return (_responseBuffer);
}

void		ClientConnection::setSentBytes(size_t bytes)
{
	this->_sentBytes = bytes;
}

void	ClientConnection::setResponseBuffer(const std::string& buffer)
{
	this->_responseBuffer = buffer;
}

HttpRequest&	ClientConnection::getRequest(void) {return this->_httpRequest;}
HttpResponse&	ClientConnection::getResponse(void) {return this->_httpResponse;}