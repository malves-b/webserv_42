#ifndef CONFIGPARSER_H
# define CONFIGPARSER_H

#include <Config.hpp>
#include <string>
#include <iostream>

class ConfigParser
{
	private:
		static std::istringstream	cleanConfigFile(std::ifstream& file);
		static void					parseServerBlock(std::istream& in, Config& config);
		static void					parseLocationBlock(std::istream& in, ServerConfig& server);
		static std::string			nextToken(std::istream& in);
		static void					expect(std::istream& in, std::string const& expected);

		ConfigParser(std::string file);
		ConfigParser(ConfigParser const& src);
		ConfigParser&				operator=(ConfigParser const& rhs);
		~ConfigParser(void);
	public:
		static Config				parseFile(std::string const& configFile); //populates the Config::_servers vector //dont allow port duplicates

};

#endif //CONFIGPARSER_H