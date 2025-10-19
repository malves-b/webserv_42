#include <config/ServerConfig.hpp>
#include <config/LocationConfig.hpp>
#include <request/RequestMethod.hpp>

ServerConfig::ServerConfig(void)
{
	this->_listenInterface = std::make_pair("127.0.0.1", "8000");
	this->_clientMaxBodysize = (1 * 1024 * 1024); // 1MB 
	//this->_indexFiles = "index.html"; //not default
	this->_autoIndex = false;
}

ServerConfig::~ServerConfig(void) {}

std::pair<std::string, std::string> const&	ServerConfig::getListenInterface(void) const
{
	return (this->_listenInterface);
}

std::string const& ServerConfig::getRoot(void) const
{
	return (this->_root);
}

std::size_t const&	ServerConfig::getClientMaxBodySize(void) const
{
	return (this->_clientMaxBodysize);
}

std::map<int, std::string> const&	ServerConfig::getErrorPage(void) const
{
	return (this->_errorPage);
}

bool	ServerConfig::getAutoIndex(void) const
{
	return (this->_autoIndex);
}

std::vector<LocationConfig> const&	ServerConfig::getLocationConfig(void) const
{
	return (this->_locations);
}

void	ServerConfig::setListenInterface(std::pair<std::string, std::string> newPair)
{
	this->_listenInterface = newPair;
}

void	ServerConfig::setRoot(std::string newRoot)
{
	this->_root = newRoot;
}

void	ServerConfig::setClientMaxBodySize(std::size_t newSize)
{
	this->_clientMaxBodysize = newSize;
}

void	ServerConfig::setErrorPage(std::map<int, std::string> newErrorPage)
{
	this->_errorPage = newErrorPage;
}

void	ServerConfig::setAutoIndex(bool newAutoIndex)
{
	this->_autoIndex = newAutoIndex;
}

void	ServerConfig::setLocationConfig(std::vector<LocationConfig> newLocation)
{
	this->_locations = newLocation;
}
