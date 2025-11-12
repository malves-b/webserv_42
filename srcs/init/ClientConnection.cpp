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

/**
 * @brief Destructor — closes the client socket if still open.
 */
ClientConnection::~ClientConnection(void)
{
	if (this->_fd != -1)
	{
		::close(this->_fd);
		Logger::instance().log(DEBUG, "ClientConnection: closed FD -> " + toString(_fd));
	}
}

/**
 * @brief Constructs a new ClientConnection with default state.
 *
 * @param config Server configuration associated with this client.
 */
ClientConnection::ClientConnection(const ServerConfig& config)
	: _fd(-1), _serverConfig(config), _sentBytes(0), _keepAlive(true),
	  _hasCgi(false), _cgiFd(-1), _cgiPid(-1), _cgiStart(0)
{
	Logger::instance().log(DEBUG, "ClientConnection: created with default state");
}

/**
 * @brief Copy constructor — duplicates configuration but not socket state.
 */
ClientConnection::ClientConnection(const ClientConnection& src)
	: _fd(-1), _serverConfig(src._serverConfig), _sentBytes(0), _keepAlive(src._keepAlive),
	  _hasCgi(false), _cgiFd(-1), _cgiPid(-1), _cgiStart(0)
{
	Logger::instance().log(DEBUG, "ClientConnection: copy-constructed");
}

/**
 * @brief Associates a socket FD with this client, closing any previous one.
 */
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

/**
 * @brief Receives incoming data from the client socket.
 *
 * Reads from the socket into an internal request buffer, and delegates parsing
 * to RequestParse. Throws on I/O error.
 *
 * @return Number of bytes received, or 0 on EOF.
 */
ssize_t	ClientConnection::recvData(void)
{
	if (_fd == -1)
		throw std::runtime_error("ClientConnection::recvData -> invalid FD (-1)");

	char buffer[4096];
	ssize_t bytesRecv = ::recv(this->_fd, buffer, sizeof(buffer), 0);

	Logger::instance().log(DEBUG, "ClientConnection::recvData bytesRecv = " + toString(bytesRecv));

	if (bytesRecv == -1)
		throw std::runtime_error("recvData: read failure");
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

/**
 * @brief Sends response data to the client.
 *
 * @param client Target connection.
 * @param sent Number of bytes already sent.
 * @param toSend Maximum number of bytes to send.
 * @return Number of bytes successfully sent.
 * @throws std::runtime_error on send failure.
 */
ssize_t	ClientConnection::sendData(ClientConnection& client, size_t sent, size_t toSend)
{
	if (_fd == -1)
		throw std::runtime_error("ClientConnection::sendData -> invalid FD (-1)");

	const std::string& response = client.getResponseBuffer();
	const char* response_string = response.c_str() + sent;

	ssize_t bytesSent = ::send(client.getFD(), response_string, toSend, MSG_NOSIGNAL);

	if (bytesSent == -1)
		throw std::runtime_error("sendData: send failure");
	if (bytesSent == 0)
	{
		Logger::instance().log(WARNING, "ClientConnection::sendData returned 0 (client closed connection?)");
		return (0);
	}

	Logger::instance().log(DEBUG, "ClientConnection::sendData sent " + toString(bytesSent) + " bytes");
	return (bytesSent);
}

/**
 * @brief Checks if the current HTTP request has been fully received and parsed.
 */
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

/**
 * @brief Clears the pending request buffer.
 */
void	ClientConnection::clearBuffer(void)
{
	_requestBuffer.clear();
	Logger::instance().log(DEBUG, "ClientConnection::clearBuffer() called");
}

int const& ClientConnection::getFD(void) const { return (this->_fd); }

size_t const& ClientConnection::getSentBytes(void) const { return (this->_sentBytes); }

std::string const& ClientConnection::getRequestBuffer(void) const { return (_requestBuffer); }

std::string const& ClientConnection::getResponseBuffer(void) const { return (_responseBuffer); }

ServerConfig const& ClientConnection::getServerConfig(void) const { return (this->_serverConfig); }

void	ClientConnection::setSentBytes(size_t bytes) { this->_sentBytes = bytes; }

void	ClientConnection::setResponseBuffer(const std::string& buffer) { this->_responseBuffer = buffer; }

bool	ClientConnection::getKeepAlive(void) const { return (this->_keepAlive); }

void	ClientConnection::setKeepAlive(bool keepAlive) { this->_keepAlive = keepAlive; }

HttpRequest&	ClientConnection::getRequest(void) { return (this->_httpRequest); }

HttpResponse&	ClientConnection::getResponse(void) { return (this->_httpResponse); }

// CGI Async Management

/**
 * @brief Returns whether a CGI process is currently associated with this client.
 */
bool	ClientConnection::hasCgi() const { return (_hasCgi); }

/**
 * @brief Alias for hasCgi(), kept for backward compatibility.
 */
bool	ClientConnection::isCgiActive() const { return (_hasCgi); }

int		ClientConnection::getCgiFd() const { return (_cgiFd); }

pid_t	ClientConnection::getCgiPid() const { return (_cgiPid); }

std::time_t	ClientConnection::getCgiStart() const { return (_cgiStart); }

std::string&	ClientConnection::cgiBuffer() { return (_cgiBuffer); }

void	ClientConnection::setCgiActive(bool v) { _hasCgi = v; }

void	ClientConnection::setCgiFd(int fd) { _cgiFd = fd; }

void	ClientConnection::setCgiPid(pid_t pid) { _cgiPid = pid; }

void	ClientConnection::setCgiStart(std::time_t t) { _cgiStart = t; }

/**
 * @brief Resets all CGI-related state (after process termination).
 */
void	ClientConnection::clearCgi()
{
	_hasCgi = false;
	_cgiFd = -1;
	_cgiPid = -1;
	_cgiStart = 0;
	_cgiBuffer.clear();
}
