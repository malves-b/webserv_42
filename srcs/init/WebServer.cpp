#include <init/WebServer.hpp>
#include <dispatcher/Dispatcher.hpp>
#include <utils/Logger.hpp>
#include <sys/socket.h> // SOMAXCONN
#include <unistd.h> //close()
#include <errno.h>
#include <poll.h>
#include <cstring>
#include <stdexcept>
#include <iostream>

#include <utils/signals.hpp> /*new*/

WebServer::WebServer(void) : _serverSocket() {}

WebServer::~WebServer(void)
{
	std::cout << "Destroying WebServer..." << std::endl;
	for (std::map<int, ClientConnection>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		close(it->first);
	}
}

void	WebServer::startServer(void)
{
	this->_serverSocket.startSocket("8080"); //this parameter will be from config file probably
	this->_serverSocket.listenConnections(SOMAXCONN);

	//start _pollFDs vector
	// the server's listening socket is always the first [0], the client ones start from index 1
	this->addToPollFD(this->_serverSocket.getFD(), POLLIN); // monitor for incoming connections
}

void	WebServer::queueClientConnections(void)
{
	std::vector<int>	newFDs = this->_serverSocket.acceptConnections(); //accepts the connections
	for (size_t j = 0; j < newFDs.size(); j++)
	{
		int	newClientFD = newFDs[j];
		if (_clients.find(newClientFD) == _clients.end()) //avoid adding duplicates
		{
			//std::cout << "queueClientConnections: fd: " << newFDs[j] << std::endl; //debug
			//new client connection
			this->_clients.insert(std::make_pair(newClientFD, ClientConnection(newClientFD)));

			//Add to pollFDs //adds the client’s file descriptor to _pollFDs so poll() will also monitor it
			this->addToPollFD(newClientFD, POLLIN);
		}
	}
}

void	WebServer::receiveRequest(size_t i)
{
	std::map<int, ClientConnection>::iterator	it;
	it = this->_clients.find(this->_pollFDs[i].fd);
	if (it != this->_clients.end()) //should I treat it in case of false?
	{
		ClientConnection	&client = it->second;
		try
		{
			ssize_t	bytesRecv = client.recvData();

			if (bytesRecv > 0 && client.completedRequest()) // >= 0?
			{
				std::cout << client.getRequestBuffer() << std::endl; //debug
				//client.setResponseBuffer(client.getResponseBuffer());
				// std::string response =
				// 	"HTTP/1.1 200 OK\r\n"
				// 	"Content-Type: text/plain\r\n"
				// 	"Content-Length: 12\r\n"
				// 	"\r\n"
				// 	"Hello World!"; //debug
				//client.setResponseBuffer(response);
				Dispatcher::dispatch(client); //real oficial
				client.clearBuffer(); //rename
				this->_pollFDs[i].events = POLLOUT; //After receiving a full request, switch events to POLLOUT
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
	Logger::instance().log(DEBUG, "[Started] WebServer::sendResponse");
	std::map<int, ClientConnection>::iterator	it;
	it = this->_clients.find(this->_pollFDs[i].fd);
	std::cout << "Response: fd: " << this->_pollFDs[i].fd << std::endl; //debug
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
				//set revents to 0 too?
				return ; //not sure?
			}
			ssize_t	bytesSent = client.sendData(client, sent, toSend);
			if (bytesSent > 0)
			{
				client.setSentBytes(sent + static_cast<size_t>(bytesSent));
				if (client.getSentBytes() == totalLen)
				{
					client.clearBuffer(); //call _responseBuffer.clear()?
					client.setSentBytes(0);
					this->_pollFDs[i].events = POLLIN; //After sending full response, switch back to POLLIN
				}
			}
			else if (bytesSent == -1)
				return ;
		}
		catch (std::exception const& e)
		{
			std::cerr << "error:" << e.what() << '\n';
			this->removeClientConnection(client.getFD(), i);
		}
	}
	Logger::instance().log(DEBUG, "[Finished] WebServer::sendResponse");
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
	Logger::instance().log(DEBUG, "[Started] WebServer::runServer");
	while (!Signals::shouldStop())
	{
		//Logger::instance().log(DEBUG, "WebServer::runServer started loop");
		int timeout = getPollTimeout(false); //update poll() timeout parameter accordingly to the presence of CGI process
		int	ready = ::poll(&this->_pollFDs[0], this->_pollFDs.size(), timeout);

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
			if (this->_pollFDs[i].revents & POLLIN) //check if POLLIN bit is set, regardless of what other bits may also be set
			{
				if (this->_pollFDs[i].fd == this->_serverSocket.getFD()) // Ready on listening socket -> accept new client
				{
					Logger::instance().log(DEBUG, "WebServer::runServer -> queue");
					this->queueClientConnections();
				}
				else //If it wasn’t the server socket, then it must be one of the client sockets
				{
					Logger::instance().log(DEBUG, "WebServer::runServer -> receive");
					this->receiveRequest(i);
				}
			}
			else if (_pollFDs[i].revents & POLLOUT)
				this->sendResponse(i);
			else if (this->_pollFDs[i].revents & (POLLERR | POLLHUP)) //POLLNVAL?
			{
				Logger::instance().log(DEBUG, "WebServer::runServer -> removing client...");
				std::map<int, ClientConnection>::iterator	it;
				it = this->_clients.find(this->_pollFDs[i].fd);
				if (it != this->_clients.end())
					this->removeClientConnection(it->second.getFD(), i);
			}
		}
		//Logger::instance().log(DEBUG, "WebServer::runServer finished loop");
	}
	Logger::instance().log(DEBUG, "[Finished] WebServer::runServer");
	
	/* NEW */
	this->cleanup();
}


/* --------------- TEST --------------- */

void WebServer::cleanup()
{
    for (std::map<int, ClientConnection>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        ::close(it->first);
    }
    _clients.clear();
    _pollFDs.clear();
}