#ifndef WEBSERVER_HPP
# define WEBSERVER_HPP

#include <vector>
#include <map>

//webserv
#include <init/ServerSocket.hpp>
#include <utils/Signals.hpp>
#include <dispatcher/CgiHandler.hpp>
#include <config/ServerConfig.hpp>
#include <config/Config.hpp>

class ClientConnection;
class WebServer
{
	private:
		Config const&					_config; //std::vector<ServerConfig>		_config;
		std::map<int, size_t>			_socketToServerIndex; // Socket FD to _config index//when poll() gives you fd = X, you can jump to configs[index] to figure out which server block this socket belongs to.
		std::vector<ServerSocket*>		_serverSocket; //ServerSocket					_serverSocket; //needs to be a vector
		std::map<int, ClientConnection>	_clients; //can also hold fd set to -1
		std::vector<struct pollfd>		_pollFDs;

		WebServer(WebServer const& src); //memmove?
		WebServer&						operator=(WebServer const& rhs); //memmove?

		std::map<int,int> _cgiFdToClientFd;
		std::map<int, CgiProcess> _cgiMap;

		void addCgiPollFd(int cgiFd);
		void removeCgiPollFd(int cgiFd);
		void handleCgiReadable(int pollIndex); // lê dados do CGI e finaliza resposta quando EOF
		void sweepCgiTimeouts();               // mata CGI estourado

	public:
		WebServer(Config const& config);
		~WebServer(void);

		void							startServer(void);
		void							runServer(void); //run loop
		void							queueClientConnections(ServerSocket& socket);
		void							addToPollFD(int fd, short events);
		void							receiveRequest(std::size_t i);
		void							sendResponse(std::size_t i);
		void							removeClientConnection(int clientFD, size_t pollFDIndex);
		void							gracefulShutdown(void);
		int								getPollTimeout(void);
};

#endif //WEBSERVER_HPP