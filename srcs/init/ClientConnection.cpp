// #include "init/ClientConnection.hpp"
// #include "request/RequestParse.hpp"
// #include "response/ResponseBuilder.hpp" // response
// #include "utils/Logger.hpp"
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

// ClientConnection::ClientConnection(int fd, ServerConfig const& config) : _fd(fd), _serverConfig(config), _sentBytes(0), _keepAlive(true) {}

// ClientConnection::ClientConnection(ClientConnection const& src) :_fd(src._fd), _serverConfig(src._serverConfig), _sentBytes(0), _keepAlive(src._keepAlive) 
// {

// }

ClientConnection::~ClientConnection(void)
{
	if (this->_fd != -1)
		::close(this->_fd);
}

ClientConnection::ClientConnection(ServerConfig const& config) : _fd(-1), _serverConfig(config), _sentBytes(0), _keepAlive(true) {} //double check

ClientConnection::ClientConnection(ClientConnection const& src) : _fd(-1), _serverConfig(src._serverConfig), _sentBytes(0), _keepAlive(src._keepAlive) 
{} //double check

void ClientConnection::adoptFD(int fd)
{
	if (_fd >= 0)
		::close(this->_fd);
	_fd = fd;
}

ssize_t	ClientConnection::recvData(void)
{
	if (_fd == -1)
		throw std::runtime_error("error: recvData: fd == -1");

	char	buffer[4096];
	ssize_t	bytesRecv;

	bytesRecv = ::recv(this->_fd, buffer, sizeof(buffer), 0);
	Logger::instance().log(DEBUG, "ClientConnection::recvData bytesRecv -> " + toString(bytesRecv));
	if (bytesRecv == -1) //ERROR
		throw std::runtime_error("recvData: read failure");
	if (bytesRecv == 0) //EOF
		return (0);
	_requestBuffer.append(buffer, bytesRecv); //If the received data has embedded nulls (unlikely in HTTP headers but possible in POST bodies), you’ll not truncate this way
	Logger::instance().log(DEBUG,
		"ClientConnection::recvData appended " + toString(bytesRecv) +
		" bytes, buffer total = " + toString(_requestBuffer.size()));
	RequestParse::handleRawRequest(_requestBuffer, _httpRequest, this->getServerConfig());
	Logger::instance().log(DEBUG, "ClientConnection::recvData request -> " + _requestBuffer);
	_requestBuffer.clear();
	return (bytesRecv);
}

ssize_t	ClientConnection::sendData(ClientConnection &client, size_t sent, size_t toSend)
{
	if (_fd == -1)
		throw std::runtime_error("error: recvData: fd == -1");

	ssize_t	bytesSent;

	const std::string& response = client.getResponseBuffer();
	const char* response_string = response.c_str() + sent;

	//MSG_NOSIGNAL to avoid SIGPIPE
	bytesSent = send(client.getFD(), response_string, toSend, 0); //double check flag
	if (bytesSent == -1)
		throw	std::runtime_error("sendData: send failure");
	if (bytesSent == 0)
		return (0);
	return (bytesSent);
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
}

void	ClientConnection::clearBuffer(void) //rename
{
	this->_requestBuffer.clear();
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

ServerConfig const&	ClientConnection::getServerConfig(void) const
{
	return (this->_serverConfig);
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
