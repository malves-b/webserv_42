#ifndef SERVERCONFIG_HPP
# define SERVERCONFIG_HPP

#include <string>
#include <vector>
#include <map>

//webserv
#include <config/LocationConfig.hpp>
#include <request/RequestMethod.hpp>

class ServerConfig
{
	private:
		std::pair<std::string, std::string>					_listenInterface; // e.g. {"127.0.0.1", "8080"} // has default *:80000 //don't allow more than on in the block
		std::string											_root; //not default and not optional //don't allow more than on in the block
		std::size_t											_clientMaxBodysize; //in bytes //default: 1mb //if set to 0, disconsider the limit
		std::map<int, std::string>							_errorPage; // e.g. {404: "errors/404.html"} //default: 404
		std::string											_indexFile; // e.g. "index.html" //not default and optional
		bool												_autoindex; // default: "off"
		std::vector<LocationConfig>							_locations; //not default and not optional

		ServerConfig&										operator=(ServerConfig const& rhs);

	public:
		ServerConfig(void);
		ServerConfig(ServerConfig const& src);
		~ServerConfig(void);

		//accessors
		std::pair<std::string, std::string> const&			getListenInterface(void) const;
		std::string const&									getRoot(void) const;
		std::size_t const&									getClientMaxBodySize(void) const;
		std::map<int, std::string> const&					getErrorPage(void) const;
		bool												getAutoindex(void) const;
		std::vector<LocationConfig> const&					getLocationConfig(void) const;
		const LocationConfig&								matchLocation(const std::string& uri) const;

		//mutators
		void												setListenInterface(std::pair<std::string, std::string>);
		void												setRoot(std::string);
		void												setClientMaxBodySize(std::size_t);
		void												setErrorPage(int, std::string );
		void												setIndexFile(std::string);
		//void												setErrorPage(std::map<int, std::string>);
		void												setAutoindex(bool);
		void												addLocation(LocationConfig& location);
};

#endif //SERVERCONFIG_HPP