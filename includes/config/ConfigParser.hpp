#ifndef CONFIGPARSER_H
# define CONFIGPARSER_H

#include <string>
#include <iostream>

//webserv
#include <config/Config.hpp>
#include <config/ServerConfig.hpp>
#include <request/RequestMethod.hpp>

class ConfigParser
{
	private:
		static std::string				cleanConfigFile(std::ifstream& file);
		static std::vector<std::string>	tokenize(std::istringstream& in);
		static void						parseServerBlock(std::vector<std::string> const& tokens, std::size_t& i, Config& config);
		static void						parseLocationBlock(std::vector<std::string> const& tokens, std::size_t& i, ServerConfig& server);
		// static std::string				nextToken(std::istream& in);
		static void						expect(std::vector<std::string> const& tokens, std::size_t& i, std::string const& expected);

		static void						parseListenInterface(std::string rawListen, ServerConfig& server);
		static void						parseClientBodySize(std::string bodySize, ServerConfig& server);
		static RequestMethod::Method	parseMethod(std::string const& token);

		ConfigParser(std::string file);
		ConfigParser(ConfigParser const& src);
		ConfigParser&					operator=(ConfigParser const& rhs);
		~ConfigParser(void);

	public:
		static Config					parseFile(std::string const& configFile); //populates the Config::_servers vector //dont allow port duplicates
};

#endif //CONFIGPARSER_H