#include <unistd.h>     // close()
#include <sys/socket.h> // recv(), send()
#include <fcntl.h>
#include <errno.h>
#include <cstring>      // strerror()
#include <stdexcept>
#include <string>
#include <init/ClientConnection.hpp>
#include <request/RequestParse.hpp>
#include <response/ResponseBuilder.hpp>
#include <utils/string_utils.hpp>
#include <utils/Logger.hpp>

ClientConnection::~ClientConnection(void)
{
	if (this->_fd != -1)
	{
		::close(this->_fd);
		Logger::instance().log(DEBUG, "ClientConnection: closed FD -> " + toString(_fd));
	}
}

ClientConnection::ClientConnection(const ServerConfig& config)
	: _fd(-1), _serverConfig(config), _sentBytes(0), _keepAlive(true)
{
	Logger::instance().log(DEBUG, "ClientConnection: created with default state");
}

ClientConnection::ClientConnection(const ClientConnection& src)
	: _fd(-1), _serverConfig(src._serverConfig), _sentBytes(0), _keepAlive(src._keepAlive)
{
	Logger::instance().log(DEBUG, "ClientConnection: copy-constructed");
}

void	ClientConnection::adoptFD(int fd)
{
	if (_fd >= 0)
	{
		Logger::instance().log(WARNING, "ClientConnection: closing previous FD -> " + toString(_fd));
		::close(_fd);
	}
	_fd = fd;
	Logger::instance().log(DEBUG, "ClientConnection: adopted new FD -> " + toString(_fd));
}

ssize_t	ClientConnection::recvData(void)
{
	if (_fd == -1)
		throw std::runtime_error("ClientConnection::recvData -> invalid FD (-1)");

	char buffer[4096];
	ssize_t bytesRecv = ::recv(this->_fd, buffer, sizeof(buffer), 0);

	Logger::instance().log(DEBUG, "ClientConnection::recvData bytesRecv = " + toString(bytesRecv));

	if (bytesRecv == -1)
	{
		throw std::runtime_error("recvData: read failure (" + std::string(strerror(errno)) + ")");
	}
	if (bytesRecv == 0)
	{
		Logger::instance().log(INFO, "ClientConnection::recvData EOF reached");
		return (0);
	}

	_requestBuffer.append(buffer, bytesRecv);

	Logger::instance().log(DEBUG,
		"ClientConnection::recvData appended " + toString(bytesRecv) +
		" bytes (total buffer size: " + toString(_requestBuffer.size()) + ")");

	RequestParse::handleRawRequest(_requestBuffer, _httpRequest, this->getServerConfig());

	Logger::instance().log(DEBUG, "ClientConnection::recvData processed request data");
	_requestBuffer.clear();

	return (bytesRecv);
}

ssize_t	ClientConnection::sendData(ClientConnection& client, size_t sent, size_t toSend)
{
	if (_fd == -1)
		throw std::runtime_error("ClientConnection::sendData -> invalid FD (-1)");

	const std::string& response = client.getResponseBuffer();
	const char* response_string = response.c_str() + sent;

	ssize_t bytesSent = ::send(client.getFD(), response_string, toSend, MSG_NOSIGNAL);

	if (bytesSent == -1)
	{
		throw std::runtime_error("sendData: send failure (" + std::string(strerror(errno)) + ")");
	}
	if (bytesSent == 0)
	{
		Logger::instance().log(WARNING, "ClientConnection::sendData returned 0 (client closed connection?)");
		return (0);
	}

	Logger::instance().log(DEBUG, "ClientConnection::sendData sent " + toString(bytesSent) + " bytes");
	return (bytesSent);
}

bool	ClientConnection::completedRequest(void)
{
	if (_httpRequest.getState() == RequestState::Complete)
	{
		Logger::instance().log(DEBUG, "ClientConnection::completedRequest -> TRUE");
		Logger::instance().log(DEBUG, "ParseError code -> " + toString(_httpRequest.getParseError()));
		return (true);
	}

	Logger::instance().log(DEBUG, "ClientConnection::completedRequest -> FALSE (state = " +
		toString(_httpRequest.getState()) + ")");
	return (false);
}

void	ClientConnection::clearBuffer(void)
{
	_requestBuffer.clear();
	Logger::instance().log(DEBUG, "ClientConnection::clearBuffer() called");
}

int const& ClientConnection::getFD(void) const
{
	return (this->_fd);
}

size_t const& ClientConnection::getSentBytes(void) const
{
	return (this->_sentBytes);
}

std::string const& ClientConnection::getRequestBuffer(void) const
{
	return (_requestBuffer);
}

std::string const& ClientConnection::getResponseBuffer(void) const
{
	return (_responseBuffer);
}

ServerConfig const& ClientConnection::getServerConfig(void) const
{
	return (this->_serverConfig);
}

void	ClientConnection::setSentBytes(size_t bytes)
{
	this->_sentBytes = bytes;
}

void	ClientConnection::setResponseBuffer(const std::string& buffer)
{
	this->_responseBuffer = buffer;
}

bool	ClientConnection::getKeepAlive(void) const
{
	return (this->_keepAlive);
}

void	ClientConnection::setKeepAlive(bool keepAlive)
{
	this->_keepAlive = keepAlive;
}

HttpRequest&	ClientConnection::getRequest(void)
{
	return (this->_httpRequest);
}

HttpResponse&	ClientConnection::getResponse(void)
{
	return (this->_httpResponse);
}
