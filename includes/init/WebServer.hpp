#ifndef WEBSERVER_HPP
# define WEBSERVER_HPP

#include "ServerSocket.hpp"
#include "config/ServerConfig.hpp"
#include "config/Config.hpp"
#include <vector>
#include <map>
#include <utils/signals.hpp>

class ClientConnection;
class WebServer
{
	private:
		Config const&					_config; //std::vector<ServerConfig>		_config;
		std::map<int, size_t>			_socketToServerIndex; // Socket FD to _config index//when poll() gives you fd = X, you can jump to configs[index] to figure out which server block this socket belongs to.
		std::vector<ServerSocket>		_serverSocket; //ServerSocket					_serverSocket; //needs to be a vector
		std::map<int, ClientConnection>	_clients; //can also hold fd set to -1
		std::vector<struct pollfd>		_pollFDs;
		//bool							run; //to handle the run loop

		WebServer(WebServer const& src); //memmove?
		WebServer&						operator=(WebServer const& rhs); //memmove?
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
		//later


		void							cleanup(void); //cleanup
};

#endif //WEBSERVER_HPP