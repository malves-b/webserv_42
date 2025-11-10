#include <config/Config.hpp>

Config::Config(void) {}

Config::Config(Config const& src) : _servers(src._servers) {}

Config::~Config(void) {}

//accessors
std::vector<ServerConfig> const&	Config::getServerConfig(void) const
{
	return (this->_servers);
}

void	Config::addServer(ServerConfig& server)
{
	this->_servers.push_back(server);
}

