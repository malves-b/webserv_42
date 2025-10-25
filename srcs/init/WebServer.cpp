// #include "init/WebServer.hpp"
#include <init/WebServer.hpp>
#include <dispatcher/Dispatcher.hpp>
#include <utils/Logger.hpp>
#include <utils/string_utils.hpp>
#include <sys/socket.h> // SOMAXCONN
#include <unistd.h> //close()
#include <errno.h>
#include <poll.h>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <fcntl.h>
#include <utils/signals.hpp> 

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


// WebServer::WebServer(void) : _serverSocket() {}

// WebServer::~WebServer(void)
// {
// 	// std::cout << "Destroying WebServer..." << std::endl;
// 	for (std::map<int, ClientConnection>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
//         ::close(it->first);
//     }
//     _clients.clear();
//     _pollFDs.clear();

// }


void	WebServer::queueClientConnections(ServerSocket &socket)
{
  std::vector<int>	newFDs = this->_serverSocket.acceptConnections(); //accepts the connections
	for (size_t j = 0; j < newFDs.size(); j++)
	{
		int	newClientFD = newFDs[j];
		if (_clients.find(newClientFD) == _clients.end()) //avoid adding duplicates
		{
			//new client connection
			size_t	serverIndex = this->_socketToServerIndex[socket.getFD()]; // get config index for this listening socket
			ServerConfig const& config = this->_config.getServerConfig()[serverIndex]; //get the config of this index
			this->_clients.insert(std::make_pair(newClientFD, ClientConnection(newClientFD, config)));
//       int flags = fcntl(newClientFD, F_GETFL, 0);
//       if (flags != -1)
//         fcntl(newClientFD, F_SETFL, flags | O_NONBLOCK);
//       if (_clients.find(newClientFD) == _clients.end()) //avoid adding duplicates
//       {
//         //std::cout << "queueClientConnections: fd: " << newFDs[j] << std::endl; //debug
//         //new client connection
//         std::pair<std::map<int, ClientConnection>::iterator, bool> res =
//           _clients.insert(std::make_pair(newClientFD, ClientConnection()));
//         ClientConnection& conn = res.first->second;
//         //this->_clients.insert(std::make_pair(newClientFD, ClientConnection(newClientFD)));
//         conn.adoptFD(newClientFD);

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
// 	if (it != this->_clients.end()) //should I treat it in case of false?
// 	{
// 		ClientConnection	&client = it->second;
// 		// try
// 		// {
// 		ssize_t	bytesRecv = client.recvData();

// 		Logger::instance().log(DEBUG, "WebServer::receiveRequest -> " + client.getRequest().getBuffer());
// 		if (bytesRecv > 0 && client.completedRequest()) // >= 0?
// 		{
// 			Logger::instance().log(DEBUG, "WebServer::receiveRequest Request Buffer: " + client.getRequestBuffer());

// 			Dispatcher::dispatch(client); //real oficial
// 			this->_pollFDs[i].events = POLLOUT; //After receiving a full request, switch events to POLLOUT
// 			client.setSentBytes(0);
// 		}
// 		else if (client.getRequest().getMeta().getExpectContinue())
// 		{
// 			Logger::instance().log(DEBUG, "WebServer::receiveRequest Expect True send");
// 			//client.clearBuffer(); //rename
// 			client.setResponseBuffer("HTTP/1.1 100 Continue\r\n\r\n");
// 			this->_pollFDs[i].events = POLLOUT; //After receiving a full request, switch events to POLLOUT
// 			client.setSentBytes(0);
// 			client.getRequest().getMeta().setExpectContinue(false);
// 		}
// 		else if (bytesRecv == 0)
// 		{
// 			Logger::instance().log(DEBUG, "WebServer::receiveRequest removeClientConnection");
// 			this->removeClientConnection(client.getFD(), i);
// 			return ;
// 		}
// 		else if (bytesRecv == -1)
// 			return ;
// 		else
// 		{
// 			Logger::instance().log(ERROR, "WebServer::sendResponse recv fatal error, closing fd=" + toString(client.getFD()));
// 			this->removeClientConnection(client.getFD(), i);
// 			return ;
// 		}

// 		// }
// 		// catch (std::exception const& e)
// 		// {
// 		// 	std::cerr << "error: " << e.what() << '\n';
// 		// 	this->removeClientConnection(client.getFD(), i);
// 		// }
	}
}

void	WebServer::sendResponse(size_t i)
{
	Logger::instance().log(DEBUG, "[Started] WebServer::sendResponse");
	std::map<int, ClientConnection>::iterator	it;
	it = this->_clients.find(this->_pollFDs[i].fd);
	Logger::instance().log(DEBUG, "WebServer::sendResponse FD -> " + toString(this->_pollFDs[i].fd));
	if (it != this->_clients.end()) //should I treat it in case of false?
	{
		ClientConnection	&client = it->second;
		// try
		// {
		size_t				totalLen = client.getResponseBuffer().length();
		size_t				sent = client.getSentBytes();
		size_t				toSend = (totalLen > sent) ? (totalLen - sent) : 0;
		Logger::instance().log(DEBUG, "WebServer::sendResponse totalLen -> "
			+ toString(totalLen)
			+ " sent -> " + toString(sent)
			+ " toSend -> " + toString(toSend));
		if (!toSend)
		{
			Logger::instance().log(DEBUG, "WebServer::sendResponse No bytes to send (buffer empty)");
			this->_pollFDs[i].events = POLLIN;
			//set revents to 0 too?
			return ; //not sure?
		}
		ssize_t	bytesSent = client.sendData(client, sent, toSend);
		if (bytesSent > 0)
		{
			Logger::instance().log(DEBUG, "WebServer::sendResponse bytesSent -> " + toString(bytesSent));
			client.setSentBytes(sent + static_cast<size_t>(bytesSent));
			if (client.getSentBytes() == totalLen)
			{
				client.clearBuffer(); //call _responseBuffer.clear()?
				client.setSentBytes(0);
				this->_pollFDs[i].events = POLLIN; //After sending full response, switch back to POLLIN
				if (!client._keepAlive)
				{
					Logger::instance().log(DEBUG, "WebServer::sendResponse keepalive false");
					this->removeClientConnection(it->second.getFD(), i);
				}
				Logger::instance().log(DEBUG, "WebServer::sendResponse back listen");
				return ;
			}
		}
		if (bytesSent == -1)
		{
			this->_pollFDs[i].events = POLLOUT;
			Logger::instance().log(DEBUG, "WebServer::sendResponse send would block, retry later");
			return ;
		}
		if (bytesSent == -2) //REMOVE THIS
		{
			Logger::instance().log(ERROR, "WebServer::sendResponse send fatal error, closing fd=" + toString(client.getFD()));
			this->removeClientConnection(it->second.getFD(), i);
			return ;
		}
		// }
		// catch (std::exception const& e)
		// {
		// 	std::string _error(e.what());
		// 	Logger::instance().log(ERROR, "WebServer::sendResponse | " + _error);
		// 	this->removeClientConnection(client.getFD(), i);
		// }
	}
	Logger::instance().log(DEBUG, "[Finished] WebServer::sendResponse");
}


void	WebServer::removeClientConnection(int clientFD, size_t pollFDIndex)
{
	Logger::instance().log(DEBUG, "WebServer::removeClientConnection fd="
		+ toString(clientFD) + " idx=" + toString(pollFDIndex));
	// std::cout << "Closing client fd=" << clientFD << std::endl;
	// std::cout << "removing client connection..." << std::endl; //debug
	this->_pollFDs.erase(_pollFDs.begin() + pollFDIndex); //erase by iterator position
	this->_clients.erase(clientFD); //erase by key // the clientConnection destructor already handles close the fd -> RAII
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
// static int	getPollTimeout(bool CGI) //refactor later
// {
// 	//time_t	now = time(NULL); //to compare with CGI start time
// 	Logger::instance().log(DEBUG, "[Started] getPollTimeout");
// 	if (!CGI)
// 		return (-1); //no timeout
// 	else
// 		return (10000); //10 seconds
// }
//

void	WebServer::runServer(void)
{
	Logger::instance().log(DEBUG, "[Started] WebServer::runServer");
	while (!Signals::shouldStop())
	{
		//Logger::instance().log(DEBUG, "WebServer::runServer started loop");
		//int timeout = getPollTimeout(false); //update poll() timeout parameter accordingly to the presence of CGI process
		int	ready = ::poll(&this->_pollFDs[0], this->_pollFDs.size(), -1);

		//Logger::instance().log(DEBUG, "WebServer::runServer after poll");

		if (ready == -1) //error
		{
			if (Signals::shouldStop())
				break;
			
			Logger::instance().log(ERROR, "WebServer::runServer -> ready == -1");
			if (errno == EINTR) //"harmless/temporary error"?
				continue ;
			std::string	errorMsg(strerror(errno));
			throw std::runtime_error("error: poll: " + errorMsg);
		}
		if (ready == 0) //poll timed out
		{
			Logger::instance().log(ERROR, "WebServer::runServer -> poll timed out");
			; //TODO //Check how long each CGI process has been running, remove zombie processes
		}

		for (ssize_t i = static_cast<ssize_t>(this->_pollFDs.size()) - 1; i >= 0 ; i--)  // Loop through all poll monitored FDs //Backward iteration avoids messing with indices when removing clients
		{
			//Logger::instance().log(DEBUG, "WebServer::runServer -> loop through poll fds");
			const short re = this->_pollFDs[i].revents;

			if (re & (POLLERR | POLLHUP | POLLRDHUP | POLLNVAL)) //POLLNVAL?
			{
				std::map<int, ClientConnection>::iterator	it;
				it = this->_clients.find(this->_pollFDs[i].fd);
				if (it != this->_clients.end())
				{
					Logger::instance().log(DEBUG, "WebServer::runServer -> removing client...");
					this->removeClientConnection(it->second.getFD(), i);
				}
				continue ;
			}
			if (re & POLLIN) //check if POLLIN bit is set, regardless of what other bits may also be set
			{
				if (this->_pollFDs[i].fd == this->_serverSocket.getFD()) // Ready on listening socket -> accept new client
				{
					Logger::instance().log(DEBUG, "WebServer::runServer -> queue");
					//With multiple servers, you need to check if the fd matches any of your server sockets
          std::map<int, size_t>::iterator it = _socketToServerIndex.find(_pollFDs[i].fd);
          if (it != _socketToServerIndex.end()) // Ready on listening socket -> accept new client
            this->queueClientConnections(this->_serverSocket[it->second]);
					continue ; // ??
				}
				else //If it wasn’t the server socket, then it must be one of the client sockets
				{
					Logger::instance().log(DEBUG, "WebServer::runServer -> receive");
					this->receiveRequest(i);
				}
			}
			if (re & POLLOUT)
				this->sendResponse(i);
		}
		//Logger::instance().log(DEBUG, "WebServer::runServer finished loop");
	}	
	// this->cleanup();
}


/* --------------- TEST --------------- */

// void WebServer::cleanup()
// {
// }
