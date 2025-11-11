#include <config/ServerConfig.hpp>
#include <config/LocationConfig.hpp>
#include <request/RequestMethod.hpp>
#include <utils/string_utils.hpp>
#include <utils/Logger.hpp>

/**
 * @brief Default constructor for ServerConfig.
 *
 * Initializes default values:
 * - listen interface: 0.0.0.0:8000
 * - client max body size: 1 MB
 * - autoindex: disabled
 */
ServerConfig::ServerConfig(void)
{
	this->_listenInterface = std::make_pair("*", "8000");
	this->_clientMaxBodysize = (1 * 1024 * 1024); // 1MB 
	this->_autoindex = false;
}

/**
 * @brief Copy constructor for ServerConfig.
 */
ServerConfig::ServerConfig(ServerConfig const& src)
	: _listenInterface(src._listenInterface),
	  _root(src._root),
	  _clientMaxBodysize(src._clientMaxBodysize),
	  _errorPage(src._errorPage),
	  _indexFile(src._indexFile),
	  _autoindex(src._autoindex),
	  _locations(src._locations)
{}

/**
 * @brief Destructor — no dynamic allocation used.
 */
ServerConfig::~ServerConfig(void) {}

/**
 * @return The IP:port pair (e.g. ("*", "8080")).
 */
std::pair<std::string, std::string> const&	ServerConfig::getListenInterface(void) const
{
	return (this->_listenInterface);
}

/**
 * @return The root directory of the server block.
 */
std::string const& ServerConfig::getRoot(void) const
{
	return (this->_root);
}

/**
 * @return Maximum allowed body size for requests, in bytes.
 */
std::size_t const&	ServerConfig::getClientMaxBodySize(void) const
{
	return (this->_clientMaxBodysize);
}

/**
 * @return Map of HTTP error codes to custom error page paths.
 */
std::map<int, std::string> const&	ServerConfig::getErrorPage(void) const
{
	return (this->_errorPage);
}

/**
 * @return True if autoindex is globally enabled for the server.
 */
bool	ServerConfig::getAutoindex(void) const
{
	return (this->_autoindex);
}

/**
 * @return Vector of all LocationConfig objects under this server.
 */
std::vector<LocationConfig> const&	ServerConfig::getLocationConfig(void) const
{
	return (this->_locations);
}

/**
 * @brief Sets the listening interface for this server.
 *
 * @param newPair A pair representing IP and port (e.g. {"127.0.0.1", "8080"}).
 */
void	ServerConfig::setListenInterface(std::pair<std::string, std::string> newPair)
{
	this->_listenInterface = newPair;
}

/**
 * @brief Defines the root directory for this server.
 */
void	ServerConfig::setRoot(std::string newRoot)
{
	this->_root = newRoot;
}

/**
 * @brief Defines the maximum allowed body size (in bytes).
 */
void	ServerConfig::setClientMaxBodySize(std::size_t newSize)
{
	this->_clientMaxBodysize = newSize;
}

/**
 * @brief Associates a custom error page with an HTTP status code.
 *
 * Example: `setErrorPage(404, "/errors/404.html");`
 */
void	ServerConfig::setErrorPage(int code, std::string path)
{
	this->_errorPage[code] = path;
}

/**
 * @brief Sets the default index file (e.g., "index.html").
 */
void	ServerConfig::setIndexFile(std::string newIndexFile)
{
	this->_indexFile = newIndexFile;
}

/**
 * @brief Enables or disables autoindex globally for the server.
 */
void	ServerConfig::setAutoindex(bool newAutoindex)
{
	this->_autoindex = newAutoindex;
}

/**
 * @brief Adds a new LocationConfig to the server.
 */
void	ServerConfig::addLocation(LocationConfig& location)
{
	this->_locations.push_back(location);
}

/**
 * @brief Finds the LocationConfig that best matches a given URI.
 *
 * - Exact match is prioritized.
 * - Fallback uses the longest prefix match.
 *
 * @param uri The requested URI (e.g. "/images/logo.png").
 * @return The most appropriate LocationConfig.
 */
const LocationConfig&	ServerConfig::matchLocation(const std::string& uri) const
{
	const LocationConfig* best = &_locations[0];
	size_t bestLen = 0;

	for (size_t i = 0; i < _locations.size(); ++i)
	{
		const std::string& locPath = _locations[i].getPath();

		// Exact match
		if (locPath == uri)
			return _locations[i];

		// Longest prefix fallback
		if (startsWith(uri, locPath) && locPath.size() > bestLen)
		{
			best = &_locations[i];
			bestLen = locPath.size();
		}
	}
	return (*best);
}
