#include <sys/socket.h>   // SOMAXCONN
#include <unistd.h>       // close()
#include <errno.h>
#include <poll.h>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <fcntl.h>
#include <sstream>
#include <sys/wait.h>
#include <init/WebServer.hpp>
#include <dispatcher/Dispatcher.hpp>
#include <response/ResponseBuilder.hpp>
#include <utils/Logger.hpp>
#include <utils/string_utils.hpp>
#include <utils/Signals.hpp>

/**
 * @brief Constructs a WebServer instance with parsed configuration.
 *
 * Initializes internal state and associates configuration objects
 * for later socket and client setup.
 *
 * @param config Parsed configuration container.
 */
WebServer::WebServer(const Config& config)
	: _config(config), _serverSocket()
{
	Logger::instance().log(INFO, "WebServer: constructed");
}

/**
 * @brief Destructor — closes all sockets and cleans up client/state structures.
 */
WebServer::~WebServer(void)
{
	Logger::instance().log(INFO, "WebServer: shutting down");

	for (std::map<int, ClientConnection>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		::close(it->first);

	_clients.clear();
	_pollFDs.clear();

	for (size_t i = 0; i < _serverSocket.size(); ++i)
	{
		if (_serverSocket[i])
		{
			delete _serverSocket[i];
			_serverSocket[i] = NULL;
		}
	}
	_serverSocket.clear();

	Logger::instance().log(INFO, "WebServer: cleanup complete");
}

/**
 * @brief Initializes and binds all listening sockets defined in the configuration.
 *
 * For each `server` block in the config, creates a listening socket,
 * binds it to the specified interface and port, and registers it into poll().
 */
void	WebServer::startServer(void)
{
	Logger::instance().log(INFO, "[Started] WebServer::startServer");

	for (size_t i = 0; i < _config.getServerConfig().size(); i++)
	{
		_serverSocket.push_back(new ServerSocket());
		ServerSocket* tmpSocket = _serverSocket.back();

		tmpSocket->startSocket(_config.getServerConfig()[i].getListenInterface().second);
		tmpSocket->listenConnections(SOMAXCONN);

		int fd = tmpSocket->getFD();
		_socketToServerIndex[fd] = i;
		addToPollFD(fd, POLLIN);

		Logger::instance().log(INFO, "WebServer: listening on FD " + toString(fd));
	}

	Logger::instance().log(INFO, "[Finished] WebServer::startServer");
}

/**
 * @brief Accepts new client connections on a given listening socket.
 *
 * Each accepted client is configured, added to `_clients`, and
 * registered in `_pollFDs` for read monitoring.
 */
void	WebServer::queueClientConnections(ServerSocket& socket)
{
	std::vector<int> newFDs = socket.acceptConnections();

	for (size_t j = 0; j < newFDs.size(); j++)
	{
		int newClientFD = newFDs[j];

		if (_clients.find(newClientFD) == _clients.end())
		{
			size_t serverIndex = _socketToServerIndex[socket.getFD()];
			const ServerConfig& config = _config.getServerConfig()[serverIndex];

			Logger::instance().log(DEBUG, "WebServer: new client connection FD -> " + toString(newClientFD));

			std::pair<std::map<int, ClientConnection>::iterator, bool> res =
				_clients.insert(std::make_pair(newClientFD, ClientConnection(config)));

			ClientConnection& conn = res.first->second;
			conn.adoptFD(newClientFD);

			addToPollFD(newClientFD, POLLIN);
		}
	}
}

/**
 * @brief Handles incoming data from a connected client.
 *
 * Reads available bytes from the socket, delegates parsing to
 * `RequestParse::handleRawRequest`, and triggers request dispatch
 * once a complete request is received.
 */
void	WebServer::receiveRequest(size_t i)
{
	std::map<int, ClientConnection>::iterator it = _clients.find(_pollFDs[i].fd);

	if (it == _clients.end())
	{
		Logger::instance().log(ERROR, "WebServer::receiveRequest: unknown client fd=" + toString(_pollFDs[i].fd));
		return ;
	}

	ClientConnection& client = it->second;

	try
	{
		ssize_t bytesRecv = client.recvData();
		Logger::instance().log(DEBUG, "WebServer::receiveRequest bytesRecv=" + toString(bytesRecv));

		if (bytesRecv > 0 && client.completedRequest())
		{
			Logger::instance().log(DEBUG, "WebServer::receiveRequest: full request received");
			Dispatcher::dispatch(client);

			if (!client.hasCgi())
			{
				_pollFDs[i].events = POLLOUT;
				client.setSentBytes(0);
			}
			else
			{
				_cgiFdToClientFd[client.getCgiFd()] = client.getFD();
				addCgiPollFd(client.getCgiFd());
				_pollFDs[i].events = POLLIN;
				_pollFDs[i].revents = 0;
			}
		}
		else if (client.getRequest().getMeta().getExpectContinue())
		{
			client.setResponseBuffer("HTTP/1.1 100 Continue\r\n\r\n");
			_pollFDs[i].events = POLLOUT;
			client.setSentBytes(0);
			client.getRequest().getMeta().setExpectContinue(false);
		}
		else if (bytesRecv == 0)
		{
			Logger::instance().log(INFO, "WebServer::receiveRequest: client disconnected");
			removeClientConnection(client.getFD(), i);
		}
	}
	catch (const std::exception& e)
	{
		Logger::instance().log(ERROR, std::string("WebServer::receiveRequest exception -> ") + e.what());
		removeClientConnection(client.getFD(), i);
	}
}

/**
 * @brief Sends buffered response data to a connected client.
 */
void	WebServer::sendResponse(size_t i)
{
	Logger::instance().log(DEBUG, "[Started] WebServer::sendResponse");

	std::map<int, ClientConnection>::iterator it = _clients.find(_pollFDs[i].fd);

	if (it == _clients.end())
	{
		Logger::instance().log(WARNING, "WebServer::sendResponse: unknown FD " + toString(_pollFDs[i].fd));
		return ;
	}

	ClientConnection& client = it->second;

	try
	{
		size_t totalLen = client.getResponseBuffer().length();
		size_t sent = client.getSentBytes();
		size_t toSend = (totalLen > sent) ? (totalLen - sent) : 0;

		if (!toSend)
		{
			_pollFDs[i].events = POLLIN;
			_pollFDs[i].revents = 0;
			return ;
		}

		ssize_t bytesSent = client.sendData(client, sent, toSend);
		if (bytesSent > 0)
		{
			client.setSentBytes(sent + static_cast<size_t>(bytesSent));

			if (client.getSentBytes() == totalLen)
			{
				client.clearBuffer();
				client.setSentBytes(0);
				_pollFDs[i].events = POLLIN;

				if (!client.getKeepAlive())
				{
					Logger::instance().log(INFO, "WebServer::sendResponse: closing connection (no keep-alive)");
					removeClientConnection(it->second.getFD(), i);
				}
			}
		}
	}
	catch (const std::exception& e)
	{
		Logger::instance().log(ERROR, std::string("WebServer::sendResponse exception -> ") + e.what());
		removeClientConnection(client.getFD(), i);
	}

	Logger::instance().log(DEBUG, "[Finished] WebServer::sendResponse");
}

/**
 * @brief Removes a client connection and cleans up associated poll entry.
 */
void	WebServer::removeClientConnection(int clientFD, size_t pollFDIndex)
{
	Logger::instance().log(DEBUG, "WebServer::removeClientConnection fd=" +
		toString(clientFD) + " idx=" + toString(pollFDIndex));

	if (pollFDIndex < _pollFDs.size())
		_pollFDs.erase(_pollFDs.begin() + pollFDIndex);

	_clients.erase(clientFD);
}

/**
 * @brief Adds a file descriptor to the monitored poll vector.
 */
void	WebServer::addToPollFD(int fd, short events)
{
	struct pollfd pfd;
	pfd.fd = fd;
	pfd.events = events;
	pfd.revents = 0;
	_pollFDs.push_back(pfd);
}

/**
 * @brief Returns poll() timeout value depending on CGI activity.
 */
int	WebServer::getPollTimeout(void)
{
	if (Signals::hasActiveCgi())
		return 100;
	return 1000;
}

/**
 * @brief Sends shutdown notice to clients and closes all sockets.
 *
 * Called on graceful termination (SIGINT). Sends HTTP 503 responses
 * before closing all connections.
 */
void	WebServer::gracefulShutdown(void)
{
	Logger::instance().log(INFO, "[Graceful shutdown initiated]");

	for (size_t i = 0; i < _serverSocket.size(); ++i)
	{
		int fd = _serverSocket[i]->getFD();
		if (fd >= 0)
			::close(fd);
	}

	for (std::map<int, ClientConnection>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		int fd = it->first;
		try
		{
			std::string shutdownMsg =
				"HTTP/1.1 503 Service Unavailable\r\n"
				"Connection: close\r\n"
				"Content-Type: text/html; charset=utf-8\r\n"
				"Content-Length: 50\r\n\r\n"
				"<html><body><h1>Server shutting down...</h1></body></html>";

			::send(fd, shutdownMsg.c_str(), shutdownMsg.size(), MSG_NOSIGNAL);
		}
		catch (...) { }

		::shutdown(fd, SHUT_RDWR);
		::close(fd);
	}

	_clients.clear();
	_pollFDs.clear();

	Logger::instance().log(INFO, "WebServer: graceful shutdown complete");
}

/**
 * @brief Main event loop — monitors sockets, dispatches requests, and handles responses.
 *
 * Uses `poll()` for multiplexing sockets, manages CGI subprocesses,
 * and performs cleanup on signals or timeouts.
 */
void	WebServer::runServer(void)
{
	Logger::instance().log(INFO, "[Started] WebServer::runServer");

	while (!Signals::shouldStop())
	{
		int timeout = getPollTimeout();
		int ready = ::poll(&_pollFDs[0], _pollFDs.size(), timeout);
		sweepCgiTimeouts();

		if (Signals::shouldStop())
			break ;

		if (ready == -1)
		{
			if (errno == EINTR)
				continue ;

			std::string errorMsg(strerror(errno));
			throw std::runtime_error("WebServer::runServer: poll failed -> " + errorMsg);
		}

		for (ssize_t i = static_cast<ssize_t>(_pollFDs.size()) - 1; i >= 0; --i)
		{
			const short re = _pollFDs[i].revents;
			const int fd = _pollFDs[i].fd;
			if (re == 0)
				continue;

			if (_cgiFdToClientFd.count(fd))
			{
				if (re & (POLLIN | POLLHUP | POLLRDHUP))
				{
					handleCgiReadable(i);
					continue;
				}
				if (re & (POLLERR | POLLNVAL))
				{
					int clientFd = _cgiFdToClientFd[fd];
					::close(fd);
					removeCgiPollFd(fd);

					std::map<int, ClientConnection>::iterator itc = _clients.find(clientFd);
					if (itc != _clients.end())
					{
						ClientConnection& c = itc->second;
						c.getResponse().setStatusCode(ResponseStatus::BadGateway);
						ResponseBuilder::build(c, c.getRequest(), c.getResponse());
						c.setResponseBuffer(ResponseBuilder::responseWriter(c.getResponse()));
						for (size_t k = 0; k < _pollFDs.size(); ++k)
						{
							if (_pollFDs[k].fd == clientFd)
							{
								_pollFDs[k].events = POLLOUT;
								_pollFDs[k].revents = 0;
								break;
							}
						}
						c.clearCgi();
					}
					continue;
				}
			}

			if (re & POLLIN)
			{
				std::map<int, size_t>::iterator it = _socketToServerIndex.find(fd);
				if (it != _socketToServerIndex.end())
				{
					queueClientConnections(*_serverSocket[it->second]);
					continue;
				}
			}

			if (re & POLLIN)
			{
				std::map<int, ClientConnection>::iterator cit = _clients.find(fd);
				if (cit != _clients.end())
				{
					receiveRequest(i);
					cit = _clients.find(fd);
					if (cit == _clients.end())
						continue;

					ClientConnection& client = cit->second;
					if (client.hasCgi())
					{
						_cgiFdToClientFd[client.getCgiFd()] = client.getFD();
						addCgiPollFd(client.getCgiFd());
						_pollFDs[i].events = POLLIN;
						_pollFDs[i].revents = 0;
					}
					continue;
				}
			}

			if (re & (POLLERR | POLLHUP | POLLRDHUP | POLLNVAL))
			{
				std::map<int, ClientConnection>::iterator itc = _clients.find(fd);
				if (itc != _clients.end())
				{
					removeClientConnection(itc->second.getFD(), i);
					continue;
				}
			}

			if (re & POLLOUT)
				sendResponse(i);
		}
	}

	gracefulShutdown();
	Logger::instance().log(INFO, "[Finished] WebServer::runServer");
}

/**
 * @brief Adds a CGI pipe descriptor to the poll list for monitoring output.
 */
void	WebServer::addCgiPollFd(int cgiFd)
{
	int flags = ::fcntl(cgiFd, F_GETFL, 0);
	if (flags != -1)
		::fcntl(cgiFd, F_SETFL, flags | O_NONBLOCK);

	struct pollfd pfd;
	pfd.fd = cgiFd;
	pfd.events = POLLIN | POLLHUP | POLLERR | POLLRDHUP | POLLNVAL;
	pfd.revents = 0;
	_pollFDs.push_back(pfd);
}

/**
 * @brief Removes a CGI FD from poll() monitoring.
 */
void	WebServer::removeCgiPollFd(int cgiFd)
{
	for (size_t i = 0; i < _pollFDs.size(); ++i)
	{
		if (_pollFDs[i].fd == cgiFd)
		{
			_pollFDs.erase(_pollFDs.begin() + i);
			break;
		}
	}
	_cgiFdToClientFd.erase(cgiFd);
}

/**
 * @brief Reads CGI process output and assembles it into the client buffer.
 *
 * When the CGI completes, the response is finalized and queued for sending.
 */
void	WebServer::handleCgiReadable(int pollIndex)
{
	int cgiFd = _pollFDs[pollIndex].fd;
	std::map<int,int>::iterator mapIt = _cgiFdToClientFd.find(cgiFd);
	if (mapIt == _cgiFdToClientFd.end())
	{
		removeCgiPollFd(cgiFd);
		return;
	}
	int clientFd = mapIt->second;

	std::map<int, ClientConnection>::iterator it = _clients.find(clientFd);
	if (it == _clients.end())
	{
		::close(cgiFd);
		removeCgiPollFd(cgiFd);
		return;
	}
	ClientConnection& client = it->second;

	char buf[4096];
	for (;;)
	{
		ssize_t n = ::read(cgiFd, buf, sizeof(buf));
		if (n > 0)
		{
			client.cgiBuffer().append(buf, n);
			continue;
		}
		if (n == 0)
		{
			::close(cgiFd);
			removeCgiPollFd(cgiFd);
			int st = 0;
			waitpid(client.getCgiPid(), &st, WNOHANG);
			Signals::unregisterCgiProcess(client.getCgiPid());

			ResponseBuilder::handleCgiOutput(client.getResponse(), client.cgiBuffer());
			ResponseBuilder::build(client, client.getRequest(), client.getResponse());
			client.setResponseBuffer(ResponseBuilder::responseWriter(client.getResponse()));

			for (size_t i = 0; i < _pollFDs.size(); ++i)
			{
				if (_pollFDs[i].fd == clientFd)
				{
					_pollFDs[i].events = POLLOUT;
					_pollFDs[i].revents = 0;
					break;
				}
			}

			client.clearCgi();
			return;
		}
		if (n < 0 && (errno == EAGAIN || errno == EINTR))
			break;
		if (n < 0)
		{
			Logger::instance().log(WARNING, "CGI read error: " + std::string(strerror(errno)));
			break;
		}
	}
}

/**
 * @brief Scans active CGI processes and terminates those exceeding timeout.
 */
void	WebServer::sweepCgiTimeouts()
{
	std::time_t now = std::time(NULL);
	std::vector<int> toKill;

	for (std::map<int,int>::iterator it = _cgiFdToClientFd.begin();
			it != _cgiFdToClientFd.end(); ++it)
	{
		int cgiFd = it->first;
		int clientFd = it->second;

		std::map<int, ClientConnection>::iterator cit = _clients.find(clientFd);
		if (cit == _clients.end())
		{
			toKill.push_back(cgiFd);
			continue;
		}
		ClientConnection& c = cit->second;
		double elapsed = difftime(now, c.getCgiStart());
		if (elapsed >= Signals::CGI_TIMEOUT_SEC)
		{
			Logger::instance().log(WARNING, "CGI timeout, killing pid=" + toString(c.getCgiPid()));
			kill(c.getCgiPid(), SIGKILL);
			int st; waitpid(c.getCgiPid(), &st, 0);
			Signals::unregisterCgiProcess(c.getCgiPid());

			c.getResponse().setStatusCode(ResponseStatus::GatewayTimeout);
			ResponseBuilder::build(c, c.getRequest(), c.getResponse());
			c.setResponseBuffer(ResponseBuilder::responseWriter(c.getResponse()));

			for (size_t i = 0; i < _pollFDs.size(); ++i)
			{
				if (_pollFDs[i].fd == clientFd)
				{
					_pollFDs[i].events = POLLOUT;
					_pollFDs[i].revents = 0;
					break;
				}
			}

			::close(cgiFd);
			toKill.push_back(cgiFd);
			c.clearCgi();
		}
	}

	for (size_t i = 0; i < toKill.size(); ++i)
		removeCgiPollFd(toKill[i]);
}
