#include <init/ServerSocket.hpp>
#include <sys/socket.h>    // socket(), setsockopt(), listen(), accept()
#include <netdb.h>         // getaddrinfo()
#include <unistd.h>        // close()
#include <fcntl.h>         // fcntl()
#include <errno.h>
#include <cstring>         // memset(), strerror()
#include <exception>
#include <stdexcept>       // runtime_error
#include <string>
#include <iostream>
#include <vector>
#include <utils/Logger.hpp>
#include <utils/string_utils.hpp>

ServerSocket::ServerSocket(void) : _fd(-1)
{
	Logger::instance().log(DEBUG, "ServerSocket: constructed (fd=-1)");
}

ServerSocket::ServerSocket(ServerSocket const& src) : _fd(src._fd)
{
	Logger::instance().log(DEBUG, "ServerSocket: copy-constructed (fd=" + toString(_fd) + ")");
}

ServerSocket::~ServerSocket(void)
{
	if (this->_fd != -1)
	{
		::close(this->_fd);
		Logger::instance().log(DEBUG, "ServerSocket: closed socket fd=" + toString(_fd));
	}
}

void	ServerSocket::startSocket(const std::string& port)
{
	int				status;
	int				socketFD;
	struct addrinfo	hints;
	struct addrinfo	*servInfo;
	struct addrinfo	*tmp;

	Logger::instance().log(INFO, "ServerSocket: initializing socket on port " + port);

	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;      // IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;  // TCP
	hints.ai_flags = AI_PASSIVE;      // Automatically fill IP

	status = ::getaddrinfo(NULL, port.c_str(), &hints, &servInfo);
	if (status != 0)
	{
		std::string	errorMsg(gai_strerror(status));
		throw std::runtime_error("ServerSocket::startSocket: getaddrinfo failed: " + errorMsg);
	}

	for (tmp = servInfo; tmp != NULL; tmp = tmp->ai_next)
	{
		socketFD = ::socket(tmp->ai_family, tmp->ai_socktype, tmp->ai_protocol);
		if (socketFD == -1)
			continue ;

		int yes = 1;
		if (::setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) != 0)
		{
			::close(socketFD);
			::freeaddrinfo(servInfo);
			std::string	errorMsg(strerror(errno));
			throw std::runtime_error("ServerSocket::startSocket: setsockopt failed: " + errorMsg);
		}

		if (::bind(socketFD, tmp->ai_addr, tmp->ai_addrlen) == 0)
			break ; // Success

		::close(socketFD);
		socketFD = -1;
	}

	::freeaddrinfo(servInfo);

	if (socketFD == -1)
	{
		std::string	errorMsg(strerror(errno));
		throw std::runtime_error("ServerSocket::startSocket: bind failed: " + errorMsg);
	}

	if (::fcntl(socketFD, F_SETFL, O_NONBLOCK) == -1)
	{
		::close(socketFD);
		std::string	errorMsg(strerror(errno));
		throw std::runtime_error("ServerSocket::startSocket: fcntl failed: " + errorMsg);
	}

	this->_fd = socketFD;

	Logger::instance().log(INFO, "ServerSocket: successfully started on port " + port);
}

void	ServerSocket::listenConnections(int backlog)
{
	if (::listen(this->_fd, backlog) == -1)
	{
		std::string	errorMsg(strerror(errno));
		throw std::runtime_error("ServerSocket::listenConnections: listen failed: " + errorMsg);
	}
	Logger::instance().log(INFO, "ServerSocket: listening with backlog=" + toString(backlog));
}

std::vector<int>	ServerSocket::acceptConnections(void)
{
	std::vector<int>	newFDs;

	while (true)
	{
		struct sockaddr_storage	clientAddr;
		socklen_t				addrSize = sizeof(clientAddr);
		int						clientFD = ::accept(this->_fd, (struct sockaddr*)&clientAddr, &addrSize);

		if (clientFD == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break ;

			std::string	errorMsg(strerror(errno));
			Logger::instance().log(ERROR, "ServerSocket::acceptConnections: accept failed: " + errorMsg);
			break ;
		}

		if (::fcntl(clientFD, F_SETFL, O_NONBLOCK) == -1)
		{
			std::string	errorMsg(strerror(errno));
			Logger::instance().log(WARNING, "ServerSocket::acceptConnections: failed to set non-blocking: " + errorMsg);
			::close(clientFD);
			continue ;
		}

		try
		{
			newFDs.push_back(clientFD);
			Logger::instance().log(DEBUG, "ServerSocket: accepted client FD=" + toString(clientFD));
		}
		catch (const std::exception& e)
		{
			Logger::instance().log(ERROR, std::string("ServerSocket::acceptConnections: exception -> ") + e.what());
			::close(clientFD);
		}
	}

	return (newFDs);
}

int	ServerSocket::getFD(void)
{
	return (this->_fd);
}
