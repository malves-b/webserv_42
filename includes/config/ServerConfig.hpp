#ifndef SERVERCONFIG_HPP
# define SERVERCONFIG_HPP

#include <LocationConfig.hpp>
#include <string>
#include <vector>

class ServerConfig
{
	private:
		std::map<std::string, std::string>	_listenInterface; //IP:Port
		std::map<int, std::string>			_errorPage;
		std::size_t							_clientMaxBodysize;
		std::string							_root; //The server block root sets the document root for the whole server
		std::vector<LocationConfig>			_locations;

		ServerConfig(ServerConfig const& src);
		ServerConfig&						operator=(ServerConfig const& rhs);
	public:
		ServerConfig(void);
		~ServerConfig(void);

		//accessors
		std::string const&					getListenPort(void) const;
};

#endif //SERVERCONFIG_HPP