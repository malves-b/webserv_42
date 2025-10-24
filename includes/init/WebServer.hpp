#ifndef WEBSERVER_HPP
# define WEBSERVER_HPP

#include "ServerSocket.hpp"
#include <vector>
#include <map>
#include <utils/signals.hpp>

class ClientConnection;
class WebServer
{
	private:
		//Config						_config; //probably const and reference //later
		ServerSocket					_serverSocket;
		std::map<int, ClientConnection>	_clients; //can also hold fd set to -1
		std::vector<struct pollfd>		_pollFDs;
		//bool							run; //to handle the run loop

		WebServer(WebServer const& src); //memmove?
		WebServer&						operator=(WebServer const& rhs); //memmove?
	public:
		WebServer(void);
		~WebServer(void);

		void							startServer(void);
		void							runServer(void); //run loop
		void							queueClientConnections(void);
		void							addToPollFD(int fd, short events);
		void							receiveRequest(size_t i);
		void							sendResponse(size_t i);
		void							removeClientConnection(int clientFD, size_t pollFDIndex);
		//later


		void							cleanup(void); //cleanup
};

#endif //WEBSERVER_HPP