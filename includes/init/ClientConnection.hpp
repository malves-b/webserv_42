#ifndef CLIENTCONNECTION_HPP
# define CLIENTCONNECTION_HPP

#include "request/HttpRequest.hpp"
#include "response/HttpResponse.hpp"
#include <string>
#include <sys/types.h>

class ServerConfig;
class ClientConnection
{
	private:
		int					_fd;
		ServerConfig const*	_serverConfig;
		std::string			_requestBuffer;
		std::string			_responseBuffer;
		size_t				_sentBytes;
		//time_t				_lastActive;
		HttpRequest			_httpRequest;
		HttpResponse		_httpResponse;

		ClientConnection&	operator=(ClientConnection const& rhs); //memmove?
	public:
		ClientConnection(int fd, ServerConfig const& config);
		ClientConnection(ClientConnection const& src); //memmove?
		~ClientConnection(void);

		// IO
		ssize_t				recvData(void);
		ssize_t				sendData(ClientConnection &client, size_t sent, size_t toSend);
		bool				completedRequest(void);
		void				clearBuffer(void);

		//accessors
		int const&			getFD(void) const;
		size_t const&		getSentBytes(void) const;
		std::string const&	getRequestBuffer(void) const;
		std::string const&	getResponseBuffer(void) const;
		ServerConfig const&	getServerConfig(void) const;
		void				setSentBytes(size_t bytes);
		void				setResponseBuffer(std::string buffer);
};

#endif //CLIENTCONNECTION_HPP