#ifndef CLIENTCONNECTION_HPP
# define CLIENTCONNECTION_HPP

#include <string>
#include <ctime>
#include <sys/types.h>

//webserv
#include <request/HttpRequest.hpp>
#include <response/HttpResponse.hpp>

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
		HttpRequest			_httpRequest;
		HttpResponse		_httpResponse;

		// CGI async
		bool				_hasCgi;
		int					_cgiFd; // read (STDOUT of CGI)
		pid_t				_cgiPid;
		std::time_t			_cgiStart;
		std::string			_cgiBuffer;

		ClientConnection&	operator=(ClientConnection const& rhs);

	public:
		// Constructors / Destructor
		ClientConnection(ServerConfig const& config);
		ClientConnection(ClientConnection const& src);
		~ClientConnection(void);

		// Core I/O
		ssize_t				recvData(void);
		ssize_t				sendData(ClientConnection &client, size_t sent, size_t toSend);
		bool				completedRequest(void);
		void				clearBuffer(void);
		void				adoptFD(int fd);
		void				setKeepAlive(bool keepAlive);

		// Accessors
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

		// CGI async support
		bool				hasCgi() const;
		bool				isCgiActive() const;
		int					getCgiFd() const;
		pid_t				getCgiPid() const;
		std::time_t			getCgiStart() const;
		std::string&		cgiBuffer();

		void				setCgiActive(bool v);
		void				setCgiFd(int fd);
		void				setCgiPid(pid_t pid);
		void				setCgiStart(std::time_t t);
		void				clearCgi();
};

#endif // CLIENTCONNECTION_HPP
