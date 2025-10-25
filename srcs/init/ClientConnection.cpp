#include "init/ClientConnection.hpp"
#include "request/RequestParse.hpp"
#include "response/ResponseBuilder.hpp" // response
#include "utils/Logger.hpp"
#include <unistd.h> //close()
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring> //strerror()
#include <stdexcept>
#include <string>

ClientConnection::ClientConnection(int fd, ServerConfig const& config) : _fd(fd), _serverConfig(&config), _sentBytes(0)
{
	this->_httpResponse.setStatusCode(ResponseStatus::NotFound);
}

ClientConnection::ClientConnection(ClientConnection const& src) : _fd(src._fd) {}

ClientConnection::~ClientConnection(void)
{
	if (this->_fd != -1)
		::close(this->_fd);
}

ssize_t	ClientConnection::recvData(void)
{
	if (_fd == -1)
		throw std::runtime_error("error: recvData: fd == -1");

	char	buffer[1024]; //4096?
	ssize_t	bytesRecv;

	bytesRecv = ::recv(this->_fd, buffer, sizeof(buffer), 0);
	if (bytesRecv == -1)
	{
		std::string	errorMsg(strerror(errno));
		throw std::runtime_error("error: recv: " + errorMsg);
	}
	if (bytesRecv == 0)
		return (0);
	_requestBuffer.append(buffer, bytesRecv); //If the received data has embedded nulls (unlikely in HTTP headers but possible in POST bodies), you’ll not truncate this way
	//std::cout << _requestBuffer << std::endl; //debug
	//RequestParse::handleRawRequest(buffer, _httpRequest); //not working properly
	return (bytesRecv);
}

ssize_t	ClientConnection::sendData(ClientConnection &client, size_t sent, size_t toSend)
{
	if (_fd == -1)
		throw std::runtime_error("error: recvData: fd == -1");

	ssize_t	bytesSent;

	bytesSent = send(client.getFD(), client.getResponseBuffer().c_str() + sent, toSend, 0);
	if (bytesSent == -1)
	{
		std::string	errorMsg(strerror(errno));
		throw	std::runtime_error("error: send: " + errorMsg);
	}
	if (bytesSent >= 0)
		return (bytesSent);
	return (bytesSent);
}

bool	ClientConnection::completedRequest(void)
{
	// if (_httpRequest.getState() == RequestState::Complete)
	// 	return (true);
	// Logger::instance().log(DEBUG, "Request not completed");

	//GET
	if (_requestBuffer.find("\r\n\r\n") != std::string::npos)
		return (true);
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
	// ResponseBuilder::run(this->_httpResponse, "");
	// std::cout << "this->_httpResponse: >>>>>" << ResponseBuilder::responseToString(this->_httpResponse) << "<<<<<" <<std::endl;
	// return (ResponseBuilder::responseToString(this->_httpResponse));
	return (_responseBuffer);
}

ServerConfig const&	ClientConnection::getServerConfig(void) const
{
	return (*this->_serverConfig);
}

void		ClientConnection::setSentBytes(size_t bytes)
{
	this->_sentBytes = bytes;
}

void	ClientConnection::setResponseBuffer(std::string buffer)
{
	this->_responseBuffer = buffer;
}

