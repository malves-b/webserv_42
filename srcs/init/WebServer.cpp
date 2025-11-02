#include <init/WebServer.hpp>
#include <dispatcher/Dispatcher.hpp>
#include <utils/Logger.hpp>
#include <utils/string_utils.hpp>
#include <utils/Signals.hpp>
#include <sys/socket.h>   // SOMAXCONN
#include <unistd.h>       // close()
#include <errno.h>
#include <poll.h>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <fcntl.h>
#include <sstream>

WebServer::WebServer(const Config& config)
	: _config(config), _serverSocket()
{
	Logger::instance().log(INFO, "WebServer: constructed");
}

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
			_pollFDs[i].events = POLLOUT;
			client.setSentBytes(0);
		}
		else if (client.getRequest().getMeta().getExpectContinue())
		{
			Logger::instance().log(DEBUG, "WebServer::receiveRequest: Expect: 100-continue");

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

void	WebServer::removeClientConnection(int clientFD, size_t pollFDIndex)
{
	Logger::instance().log(DEBUG, "WebServer::removeClientConnection fd=" +
		toString(clientFD) + " idx=" + toString(pollFDIndex));

	if (pollFDIndex < _pollFDs.size())
		_pollFDs.erase(_pollFDs.begin() + pollFDIndex);

	_clients.erase(clientFD);
}

void	WebServer::addToPollFD(int fd, short events)
{
	struct pollfd pfd;
	pfd.fd = fd;
	pfd.events = events;
	pfd.revents = 0;
	_pollFDs.push_back(pfd);
}

int	WebServer::getPollTimeout(void)
{
	if (Signals::hasActiveCgi())
	{
		Logger::instance().log(DEBUG, "WebServer::getPollTimeout -> CGI active (10s)");
		return (10000); // Wait 10 seconds when CGI is running
	}
	return (1000); // Default 1s
}

void	WebServer::gracefulShutdown(void)
{
	Logger::instance().log(INFO, "[Graceful shutdown initiated]");

	// 1. Close listening sockets (stop accepting new connections)
	for (size_t i = 0; i < _serverSocket.size(); ++i)
	{
		int fd = _serverSocket[i]->getFD();
		if (fd >= 0)
			::close(fd);
	}

	// 2. Send 503 to active clients and close sockets
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

void	WebServer::runServer(void)
{
	Logger::instance().log(INFO, "[Started] WebServer::runServer");

	while (!Signals::shouldStop())
	{
		int timeout = getPollTimeout();
		int ready = ::poll(&_pollFDs[0], _pollFDs.size(), timeout);

		if (Signals::hasActiveCgi())
			Signals::checkCgiTimeouts();

		if (Signals::shouldStop())
			break ;

		if (ready == -1)
		{
			if (errno == EINTR)
				continue ;

			std::string errorMsg(strerror(errno));
			throw std::runtime_error("WebServer::runServer: poll failed -> " + errorMsg);
		}

		for (ssize_t i = static_cast<ssize_t>(_pollFDs.size()) - 1; i >= 0; i--)
		{
			const short re = _pollFDs[i].revents;

			if (re & POLLIN)
			{
				std::map<int, size_t>::iterator it = _socketToServerIndex.find(_pollFDs[i].fd);
				if (it != _socketToServerIndex.end())
					queueClientConnections(*_serverSocket[it->second]);
				else
					receiveRequest(i);
			}
			else if (re & (POLLERR | POLLHUP | POLLRDHUP | POLLNVAL))
			{
				std::map<int, ClientConnection>::iterator it = _clients.find(_pollFDs[i].fd);
				if (it != _clients.end())
					removeClientConnection(it->second.getFD(), i);
			}
			else if (re & POLLOUT)
				sendResponse(i);
		}
	}

	gracefulShutdown();
	Logger::instance().log(INFO, "[Finished] WebServer::runServer");
}
