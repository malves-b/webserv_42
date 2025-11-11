#include <config/Config.hpp>

/**
 * @brief Default constructor — initializes an empty configuration container.
 */
Config::Config(void) {}

/**
 * @brief Copy constructor — performs a deep copy of server configurations.
 *
 * @param src Source configuration to copy.
 */
Config::Config(Config const& src) : _servers(src._servers) {}

/**
 * @brief Destructor — performs cleanup (no dynamic resources used).
 */
Config::~Config(void) {}

/**
 * @brief Returns the list of all parsed server configurations.
 *
 * Each entry represents a single `server` block from the configuration file.
 *
 * @return Constant reference to the internal vector of ServerConfig objects.
 */
std::vector<ServerConfig> const&	Config::getServerConfig(void) const
{
	return (this->_servers);
}

/**
 * @brief Adds a new server configuration to the configuration set.
 *
 * @param server Reference to a ServerConfig object to append.
 */
void	Config::addServer(ServerConfig& server)
{
	this->_servers.push_back(server);
}
