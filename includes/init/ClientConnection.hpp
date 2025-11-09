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
		ServerConfig const&	_serverConfig;
		std::string			_requestBuffer;
		std::string			_responseBuffer;
		size_t				_sentBytes;
		bool				_keepAlive;
		//time_t				_lastActive;
		HttpRequest			_httpRequest;
		HttpResponse		_httpResponse;

		ClientConnection&	operator=(ClientConnection const& rhs); //memmove?

	public:
		ClientConnection(ServerConfig const& config);
		ClientConnection(ClientConnection const& src);
		~ClientConnection(void);

		ssize_t				recvData(void);
		ssize_t				sendData(ClientConnection &client, size_t sent, size_t toSend);
		bool				completedRequest(void);
		void				clearBuffer(void);
		void				adoptFD(int fd);
		void				setKeepAlive(bool keepAlive);

		//accessors
		int const&			getFD(void) const;
		size_t const&		getSentBytes(void) const;
		std::string const&	getRequestBuffer(void) const;
		std::string const&	getResponseBuffer(void) const;
		ServerConfig const&	getServerConfig(void) const;
		bool				getKeepAlive(void) const;
		void				setSentBytes(size_t bytes);
		void				setResponseBuffer(const std::string& buffer);

		HttpRequest&		getRequest(void);
		HttpResponse&		getResponse(void);
};

#endif //CLIENTCONNECTION_HPP