#include <config/ServerConfig.hpp>
#include <config/LocationConfig.hpp>
#include <request/RequestMethod.hpp>
#include <utils/string_utils.hpp>

ServerConfig::ServerConfig(void)
{
	this->_listenInterface = std::make_pair("*", "8000");
	this->_clientMaxBodysize = (1 * 1024 * 1024); // 1MB 
	//this->_indexFiles = "index.html"; //not default, not mandatory
	this->_autoindex = false;
}

ServerConfig::ServerConfig(ServerConfig const& src)
	: _listenInterface(src._listenInterface),
	  _root(src._root),
	  _clientMaxBodysize(src._clientMaxBodysize),
	  _errorPage(src._errorPage),
	  _indexFile(src._indexFile),
	  _autoindex(src._autoindex),
	  _locations(src._locations)
{}

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

bool	ServerConfig::getAutoindex(void) const
{
	return (this->_autoindex);
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

void	ServerConfig::setErrorPage(int code, std::string path)
{
	this->_errorPage[code] = path;
}

void	ServerConfig::setIndexFile(std::string newIndexFile)
{
	this->_indexFile = newIndexFile;
}

void	ServerConfig::setAutoindex(bool newAutoindex)
{
	this->_autoindex = newAutoindex;
}

void	ServerConfig::addLocation(LocationConfig& location)
{
	this->_locations.push_back(location);
}

const LocationConfig&	ServerConfig::mathLocation(const std::string& uri) const
{
	const LocationConfig* best = &_locations[0];
	size_t bestLen = 0;

	for (size_t i = 0; i < _locations.size(); ++i)
	{
		const std::string& locPath = _locations[i].getPath();

		//Match
		if (locPath == uri)
			return _locations[i];

		//Fallback
		if (startsWith(uri, locPath) && locPath.size() > bestLen)
		{
			best = &_locations[i];
			bestLen = locPath.size();
		}
	}
	return (*best);
}