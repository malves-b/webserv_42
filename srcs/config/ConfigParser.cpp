#include <config/ConfigParser.hpp>
#include <fstream> 
#include <sstream>


std::string	ConfigParser::nextToken(std::istream& in)
{
	std::string	token;
	in >> token;
	if (!in)
		throw std::runtime_error("Unexpected end of file");
	return (token);
}

void	ConfigParser::expect(std::istream& in, std::string const& expected)
{
	std::string	token = nextToken(in);
	if (token != expected)
		throw std::runtime_error("Expected '" + expected + "', got '" + token + "'");
}

void	ConfigParser::parseServerBlock(std::istream& in, Config& config)
{
	expect(in, "{");

	ServerConfig	server;
	std::string		token;

	while (in >> token)
	{
		if (token == "}")
			break ;
		else if (token == "listen")
	}

}

std::istringstream	ConfigParser::cleanConfigFile(std::ifstream& file)
{
	std::stringstream	clean;
	std::string			line;

	while (std::getline(file, line))
	{
		std::string::size_type	commentPos = line.find('#');
		if (commentPos != std::string::npos)
			line.erase(commentPos); //erase comments
		if (!line.empty())
			clean << line << '\n';
	}
	file.close();
	std::istringstream res(clean.str());
	return (res);
}


Config	ConfigParser::parseFile(std::string const& configFile)
{
	std::ifstream	rawFile(configFile.c_str());
	if (!rawFile.is_open())
		throw std::runtime_error("Filed to open config file: " + configFile);

	std::istringstream file = cleanConfigFile(rawFile);

	Config		config;
	std::string	token;

	while (file >> token) //automatically skips all normal whitespace between tokens (spaces, tabs, newlines)
	{
		if (token == "server")
			parseServerBlock(file, config);
		else
			throw std::runtime_error("Unknown directive: " + token);
	}
	return (config);
}