#ifndef SERVERSOCKET_HPP
# define SERVERSOCKET_HPP

#include "ClientConnection.hpp"
#include <string>
#include <vector>

class ServerSocket
{
	private:
		int					_fd;

		ServerSocket&					operator=(ServerSocket const& rhs); //memmove?
	public:
		ServerSocket(void);
		ServerSocket(ServerSocket const& src); //memmove?
		~ServerSocket(void);

		void				startSocket(std::string const& port);
		void				listenConnections(int backlog);
		std::vector<int>	acceptConnections(void);
		//std::vector<ClientConnection>	acceptConnections(void); //accepts clients, producing a vector of ClientConnection

		//accesor
		int					getFD(void);
};

#endif //SERVERSOCKET_HPP