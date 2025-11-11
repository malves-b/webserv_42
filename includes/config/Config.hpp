#ifndef CONFIG_HPP
# define CONFIG_HPP

#include <vector>

//webserv
#include <config/ServerConfig.hpp>

class Config
{
	private:
		std::vector<ServerConfig>			_servers;
		Config&	operator=(Config const& rhs);

	public:
		Config(void);
		Config(Config const& src);
		~Config(void);

		//accessors
		std::vector<ServerConfig> const&	getServerConfig(void) const;

		//mutators
		void								addServer(ServerConfig& server);

		//void validatePorts(void) const; //throws exception
		//global settings (timeouts, worker count, CGI config, etc.)?
};

#endif //CONFIG_HPP