#include <init/ServerSocket.hpp>
#include <sys/socket.h> //socket(), setsockopt(), listen()
#include <netdb.h> //getaddrinfo()
#include <unistd.h> //close() //not sure if there is a C++ alternative
#include <fcntl.h> //fcntl()
#include <errno.h>
#include <cstring> //memset(), strerror()
#include <exception>
#include <stdexcept> //runtime_error
#include <string>
#include <iostream>

ServerSocket::ServerSocket(void) : _fd(-1) {}

ServerSocket::ServerSocket(ServerSocket const& src) : _fd(src._fd) {} //memmove?

ServerSocket::~ServerSocket(void)
{
	if (this->_fd != -1)
		::close(this->_fd);
}

void	ServerSocket::startSocket(std::string const& port)
{
	int				status;
	int				socketFD;
	struct addrinfo	hints;
	struct addrinfo	*servInfo;
	struct addrinfo	*tmp;

	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; //IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; //TCP
	hints.ai_flags = AI_PASSIVE; //Fill IP for me

	if ((status = ::getaddrinfo(NULL, port.c_str(), &hints, &servInfo)) != 0)
	{
		std::string	errorMsg(gai_strerror(status));
		throw std::runtime_error("error: getaddrinfo: " + errorMsg);
	}
	/* getaddrinfo() returns a list of address structures.
	Try each address until we successfully bind(2).
	If socket(2) (or bind(2)) fails, we (close the socket
	and) try the next address. */
	for (tmp = servInfo; tmp != NULL; tmp = tmp->ai_next)
	{
		socketFD = ::socket(tmp->ai_family, tmp->ai_socktype, tmp->ai_protocol);
		if (socketFD == -1)
			continue ;
		int	yes = 1;
		//SO_REUSEADDR and process timing can make duplicate binds “sometimes work”. 
		//if SO_REUSEADDR is set, the OS might temporarily allow 
		//a bind to a port that appears free but is still “in use” in some edge cases 
		//(e.g., TIME_WAIT sockets). This is outside our control
		if (::setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) != 0)
		{
			::freeaddrinfo(servInfo);
			::close(socketFD);
			std::string	errorMsg(strerror(errno));
			throw std::runtime_error("error: setsockopt: " + errorMsg);
		}
		if (::bind(socketFD, tmp->ai_addr, tmp->ai_addrlen) == 0) //If two server configs specify the same port (say 8080 twice), your second bind() will fail with EADDRINUSE
			break ; //sucess
		::close(socketFD);
		socketFD = -1;
	}
	::freeaddrinfo(servInfo);
	if (socketFD == -1)
	{
		std::string	errorMsg(strerror(errno));
		throw std::runtime_error("error: bind: " + errorMsg);
	}
	if (::fcntl(socketFD, F_SETFL, O_NONBLOCK) == -1)
	{
		::close(socketFD);
		std::string	errorMsg(strerror(errno));
		throw std::runtime_error("error: fcntl: " + errorMsg);
	}
	this->_fd = socketFD;
}

void	ServerSocket::listenConnections(int backlog)
{
	if (::listen(this->_fd, backlog) == -1)
	{
		std::string	errorMsg(strerror(errno));
		throw std::runtime_error("listen: listenConnections: " + errorMsg);
	}
}

std::vector<int>	ServerSocket::acceptConnections(void)
{
	std::vector<int>	newFDs;

	//the kernel doesn’t guarantee that there’s exactly connection to be accepted.
	//There may be multiple connections queued in the backlog.
	//This drains the kernel’s pending connection queue in one go.
	//You don’t need to wait for another poll() cycle to accept the remaining queued clients.
	while (true)
	{
		struct sockaddr_storage	clientAddr;
		socklen_t				addrSize;
		int						clientFD;

		addrSize = sizeof(clientAddr);
		clientFD = accept(this->_fd, (struct sockaddr*)&clientAddr, &addrSize);

		if (clientFD == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK) //no client ready yet -> break
				break ;
			std::string	errorMsg(strerror(errno));
			std::cerr << "error: accept: " << errorMsg << '\n';
			break ; //will return fds anyway?
		}
		//Set client sockets to non-blocking. The new fd got from accept() may not 
		//inherit non-blocking on all platforms
		if (::fcntl(clientFD, F_SETFL, O_NONBLOCK) == -1)
		{
			::close(clientFD);
			std::string	errorMsg(strerror(errno));
			std::cerr << "error: fcntl: " << errorMsg << '\n';
			continue ; //skip this client
		}
		try
		{
			newFDs.push_back(clientFD);
		}
		catch (std::exception const& e)
		{
			::close(clientFD);
			std::cerr << "error: " << e.what() << '\n';
		}
	}
	return (newFDs);
}

int	ServerSocket::getFD(void)
{
	return (this->_fd);
}
