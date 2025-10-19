#include <init/WebServer.hpp>
#include <sys/socket.h> // SOMAXCONN
#include <unistd.h> //close()
#include <errno.h>
#include <poll.h>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <sstream>

WebServer::WebServer(Config const& config) : _config(config), _serverSocket() {}

WebServer::~WebServer(void){}

void	WebServer::startServer(void)
{
	for (size_t i = 0; i < this->_config.getServerConfig().size(); i++)
	{
		ServerSocket	tmpSocket;

		tmpSocket.startSocket(this->_config.getServerConfig()[i].getListenInterface().second);
		tmpSocket.listenConnections(SOMAXCONN);

		this->_socketToServerIndex[tmpSocket.getFD()] = i; //index in config's ServerConfig vector
		this->_serverSocket.push_back(tmpSocket); //index in ServerSocket is equal to index in config's ServerConfig vector
		this->addToPollFD(tmpSocket.getFD(), POLLIN); // monitor for incoming connections
	}
}

void	WebServer::queueClientConnections(ServerSocket &socket)
{
	std::vector<int>	newFDs = socket.acceptConnections(); //accepts the connections
	for (size_t j = 0; j < newFDs.size(); j++)
	{
		int	newClientFD = newFDs[j];
		if (_clients.find(newClientFD) == _clients.end()) //avoid adding duplicates
		{
			//new client connection
			size_t	serverIndex = this->_socketToServerIndex[socket.getFD()]; // get config index for this listening socket
			ServerConfig const& config = this->_config.getServerConfig()[serverIndex]; //get the config of this index
			this->_clients.insert(std::make_pair(newClientFD, ClientConnection(newClientFD, config)));

			//Add to pollFDs //adds the client’s file descriptor to _pollFDs so poll() will also monitor it
			this->addToPollFD(newClientFD, POLLIN);
		}
	}
}

void	WebServer::receiveRequest(size_t i)
{
	std::map<int, ClientConnection>::iterator	it;
	it = this->_clients.find(this->_pollFDs[i].fd);
	if (it != this->_clients.end()) //found client
	{
		ClientConnection	&client = it->second;
		try
		{
			ssize_t	bytesRecv = client.recvData();

			if (bytesRecv > 0 && client.completedRequest())
			{
				std::cout << client.getRequestBuffer() << std::endl; //debug
				client.setResponseBuffer(client.getResponseBuffer());
				std::string response =
					"HTTP/1.1 200 OK\r\n"
					"Content-Type: text/plain\r\n"
					"Content-Length: 12\r\n"
					"\r\n"
					"Hello World!"; //debug
				client.setResponseBuffer(response);
				client.clearBuffer(); //rename
				this->_pollFDs[i].events = POLLOUT; //After receiving a full request, switch events to POLLOUT
				this->_pollFDs[i].revents = 0;
				client.setSentBytes(0);
			}
			else if (bytesRecv == 0)
				this->removeClientConnection(client.getFD(), i);
		}
		catch (std::exception const& e)
		{
			std::cerr << "error: " << e.what() << '\n';
			this->removeClientConnection(client.getFD(), i);
		}
	}
}

void	WebServer::sendResponse(size_t i)
{
	std::map<int, ClientConnection>::iterator	it;
	it = this->_clients.find(this->_pollFDs[i].fd);
	//std::cout << "Response: fd: " << this->_pollFDs[i].fd << std::endl; //debug
	if (it != this->_clients.end()) //should I treat it in case of false?
	{
		ClientConnection	&client = it->second;
		try
		{
			size_t				totalLen = client.getResponseBuffer().length();
			size_t				sent = client.getSentBytes();
			size_t				toSend = (totalLen > sent) ? (totalLen - sent) : 0;

			if (!toSend)
			{
				this->_pollFDs[i].events = POLLIN;
				return ;
			}
			ssize_t	bytesSent = client.sendData(client, sent, toSend);
			if (bytesSent > 0)
			{
				client.setSentBytes(sent + static_cast<size_t>(bytesSent));
				if (client.getSentBytes() == totalLen)
				{
					//client.clearBuffer(); //call _responseBuffer.clear()?
					client.setSentBytes(0);
					this->_pollFDs[i].events = POLLIN; //After sending full response, switch back to POLLIN
					this->_pollFDs[i].revents = 0;
				}
			}
			else if (bytesSent == -1) //Checking the value of errno to adjust the server behaviour is strictly forbidden after performing a read or write operation.
				return ; //should I thorw exception
		}
		catch (std::exception const& e)
		{
			std::cerr << "error:" << e.what() << '\n';
			this->removeClientConnection(client.getFD(), i);
		}
	}
}

void	WebServer::removeClientConnection(int clientFD, size_t pollFDIndex)
{
	// std::cout << "Closing client fd=" << clientFD << std::endl;
	// std::cout << "removing client connection..." << std::endl; //debug
	this->_clients.erase(clientFD); //erase by key // the clientConnection destructor already handles close the fd -> RAII
	this->_pollFDs.erase(_pollFDs.begin() + pollFDIndex); //erase by iterator position
}

void	WebServer::addToPollFD(int fd, short events)
{
	struct pollfd	pollFD;
	pollFD.fd = fd;
	pollFD.events = events;
	pollFD.revents = 0;
	this->_pollFDs.push_back(pollFD);
}

//HELPER FUNCTION
static int	getPollTimeout(bool CGI) //refactor later
{
	//time_t	now = time(NULL); //to compare with CGI start time

	if (!CGI)
		return (-1); //no timeout
	else
		return (10000); //10 seconds
}
//

void	WebServer::runServer(void)
{
	while (true)
	{
		//subject: A request to your server should never hang indefinitely
		int timeout = getPollTimeout(false); //update poll() timeout parameter accordingly to the presence of CGI process
		int	ready = ::poll(&this->_pollFDs[0], this->_pollFDs.size(), timeout);

		if (ready == -1) //error
		{
			if (errno == EINTR) //"harmless/temporary error"?
				continue ;
			std::string	errorMsg(strerror(errno));
			throw std::runtime_error("error: poll: " + errorMsg);
		}
		if (ready == 0) //poll timed out
		{
			; //TODO //Check how long each CGI process has been running, remove zombie processes
		}

		for (ssize_t i = static_cast<ssize_t>(this->_pollFDs.size()) - 1; i >= 0 ; i--)  // Loop through all poll monitored FDs //Backward iteration avoids messing with indices when removing clients
		{
			if (this->_pollFDs[i].revents & POLLIN) //check if POLLIN bit is set, regardless of what other bits may also be set
			{
				//With multiple servers, you need to check if the fd matches any of your server sockets
				std::map<int, size_t>::iterator it = _socketToServerIndex.find(_pollFDs[i].fd);
				if (it != _socketToServerIndex.end()) // Ready on listening socket -> accept new client
					this->queueClientConnections(this->_serverSocket[it->second]);
				else
					this->receiveRequest(i);
			}
			else if (_pollFDs[i].revents & POLLOUT)
				this->sendResponse(i);
			else if (this->_pollFDs[i].revents & (POLLERR | POLLHUP)) //POLLNVAL?
			{
				//std::cout << "removing client..." << std::endl;
				std::map<int, ClientConnection>::iterator	it;
				it = this->_clients.find(this->_pollFDs[i].fd);
				if (it != this->_clients.end())
					this->removeClientConnection(it->second.getFD(), i);
			}
		}
	}
}
