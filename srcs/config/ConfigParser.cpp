#include "config/ConfigParser.hpp"
#include "config/ServerConfig.hpp"
#include <fstream> 
#include <sstream>
#include <locale>
#include <limits>
#include <cmath>

RequestMethod::Method	ConfigParser::parseMethod(std::string const& token)
{
	if (token == "get")
		return RequestMethod::GET;
	else if (token == "post")
		return RequestMethod::POST;
	else if (token == "delete")
		return RequestMethod::DELETE;
	else
		throw std::runtime_error("Unknown HTTP method: " + token);
}

void	ConfigParser::parseLocationBlock(std::vector<std::string> const& tokens, std::size_t& i, ServerConfig& server)
{
	bool	hasMethods = false;
	bool	hasReturn = false;
	bool	hasUploadPath = false;
	bool	hasUploadEnabled = false;
	bool	hasCgiPath = false;

	if (i + 2 >= tokens.size())
		throw std::runtime_error("Missing path for location directive");

	std::string	path = tokens[i + 1];
	if (tokens[i + 2] != "{")
		throw std::runtime_error("Expected '{' after location path");

	LocationConfig location(path);
	i += 3; // move to first directive inside block

	while (i < tokens.size())
	{
		std::string	token = tokens[i];

		if (tokens[i] == "}")
			break ;

		if (token == "root")
		{
			if (location.getHasRoot())
				throw std::runtime_error("Duplicate root directive in " + path);
			if (i + 1 >= tokens.size())
				throw std::runtime_error("Missing argument for root in " + path);
			location.setRoot(tokens[i + 1]);
			i += 2;
		}
		else if (token == "index")
		{
			if (location.getHasIndexFiles())
				throw std::runtime_error("Duplicate index directive in " + path);
			if (i + 1 >= tokens.size())
				throw std::runtime_error("Missing argument for index in " + path);
			location.setIndex(tokens[i + 1]);
			i += 2;
		}
		else if (token == "autoindex")
		{
			if (location.getHasAutoIndex())
				throw std::runtime_error("Duplicate autoindex directive in " + path);
			if (i + 1 >= tokens.size())
				throw std::runtime_error("Missing argument for autoindex in " + path);
			std::string flag = tokens[i + 1];
			if (flag == "on")
				location.setAutoindex(true);
			else if (flag == "off")
				location.setAutoindex(false);
			else
				throw std::runtime_error("Invalid value for autoindex: must be 'on' or 'off'");
			i += 2;
		}
		else if (token == "methods")
		{
			if (hasMethods)
				throw std::runtime_error("Duplicate methods directive in " + path);
			std::vector<RequestMethod::Method> methods;
			while (i + 1 < tokens.size() && tokens[i + 1] != ";")
			{
				methods.push_back(parseMethod(tokens[i + 1]));
				++i;
			}
			if (i + 1 >= tokens.size() || tokens[i + 1] != ";")
			throw std::runtime_error("Missing ';' after methods list in location " + path);
			location.setMethods(methods);
			hasMethods = true;
			i++;
		}
		else if (token == "return")
		{
			if (hasReturn)
				throw std::runtime_error("Duplicate return directive in " + path);
			if (i + 2 >= tokens.size())
				throw std::runtime_error("Missing arguments for return in " + path);

			int code = atoi(tokens[i + 1].c_str());
			std::string url = tokens[i + 2];
			location.setReturn(std::make_pair(code, url));
			hasReturn = true;
			i += 3;
		}
		else if (token == "upload_path")
		{
			if (hasUploadPath)
				throw std::runtime_error("Duplicate upload_path directive in " + path);
			if (i + 1 >= tokens.size())
				throw std::runtime_error("Missing argument for upload_path in " + path);
			location.setUploadPath(tokens[i + 1]);
			hasUploadPath = true;
			i += 2;
		}
		else if (token == "upload_enabled")
		{
			if (hasUploadEnabled)
				throw std::runtime_error("Duplicate upload_enabled directive in " + path);
			if (i + 1 >= tokens.size())
				throw std::runtime_error("Missing argument for upload_enabled in " + path);
			std::string flag = tokens[i + 1];
			if (flag == "on")
				location.setUploadEnabled(true);
			else if (flag == "off")
				location.setUploadEnabled(false);
			else
				throw std::runtime_error("Invalid value for upload_enabled: must be 'on' or 'off'");
			location.setUploadEnabled(true);
			hasUploadEnabled = true;
			i += 2;
		}
		else if (token == "cgi_path")
		{
			if (hasCgiPath)
				throw std::runtime_error("Duplicate cgi_path directive in " + path);
			if (i + 1 >= tokens.size())
				throw std::runtime_error("Missing argument for cgi_path in " + path);
			location.setCgiPath(tokens[i + 1]);
			hasCgiPath = true;
			i += 2;
		}
		else if (token == "cgi_extension")
		{
			if (i + 2 >= tokens.size())
				throw std::runtime_error("Missing arguments for cgi_extension in " + path);
			location.addCgiExtension(tokens[i + 1], tokens[i + 2]);
			i += 3;
		}
		else if (token == "location")
			throw std::runtime_error("Location nesting is not allowed in location directive");
		else
			throw std::runtime_error("Unknown directive in location block: " + token + " inside location " + path);
		if (tokens[i] != "}")
			expect(tokens, i, ";");
	}
	if (i >= tokens.size() || tokens[i] != "}")
		throw std::runtime_error("Missing '}' at end of location block");
	i += 1; // move past "}"
	server.addLocation(location);
}

void	ConfigParser::expect(std::vector<std::string> const& tokens, std::size_t& i, std::string const& expected)
{
	if (i >= tokens.size())
		throw std::runtime_error("Unexpected end of tokens, expected'" + expected + "'");
	if (tokens[i] != expected)
		throw std::runtime_error("Expected '" + expected + "', got '" + tokens[i] + "'");
	++i;
}

void	ConfigParser::parseListenInterface(std::string rawListen, ServerConfig& server)
{
	std::string	ip;
	std::string	port;

	std::string::size_type	coloPos = rawListen.find(':');
	if (coloPos == std::string::npos)
	{
		ip = "*";
		port = rawListen;
	}
	ip = rawListen.substr(0, coloPos);
	port = rawListen.substr(coloPos + 1);

	server.setListenInterface(std::make_pair(ip, port));
}

void	ConfigParser::parseClientBodySize(std::string bodySize, ServerConfig& server)
{
	for (std::string::iterator it = bodySize.begin(); it != bodySize.end(); ++it)
		*it = std::tolower(*it);

	char		*endPtr;
	long		nbr;

	nbr = strtol(bodySize.c_str(), &endPtr, 10);
	if (nbr < 0)
		throw std::runtime_error("Negative body size not allowed");

	std::string	suffix = bodySize.substr(endPtr - bodySize.c_str());
	long	multiplier = 1;

	if (suffix == "" || suffix == "b")
		multiplier = 1;
	else if (suffix == "k" || suffix == "kb")
		multiplier = 1024L;
	else if (suffix == "m" || suffix == "mb")
		multiplier = 1024L * 1024L;
	else if (suffix == "g" || suffix == "gb")
		multiplier = 1024L * 1024L * 1024L;
	else
		throw std::runtime_error("Invalid size suffix: " + suffix);

	unsigned long long	size = static_cast<unsigned long long>(nbr) * multiplier;

	if (size > static_cast<unsigned long long>(std::numeric_limits<std::size_t>::max()))
		throw std::runtime_error("Client body size too large");
	server.setClientMaxBodySize(static_cast<std::size_t>(size));
}


void	ConfigParser::parseServerBlock(std::vector<std::string> const& tokens, std::size_t& i, Config& config)
{
	expect(tokens, ++i, "{"); //skip {

	ServerConfig	server;
	bool			hasListen = false;
	bool			hasRoot = false;
	bool			hasIndex = false;
	bool			hasBodySize = false;
	bool			hasAutoIndex = false;
	bool			hasLocation = false;

	while (i < tokens.size())
	{
		if (tokens[i] == "}")
			break ;
		std::string	token = tokens[i];
		if (token == "listen")
		{
			if (hasListen)
				throw std::runtime_error("Duplicate listen directive");
			if (i + 1 >= tokens.size())
				throw std::runtime_error("Missing argument for 'listen'");
			parseListenInterface(tokens[i + 1], server);
			hasListen = true;
			i += 2;
		}
		else if (token == "root")
		{
			if (hasRoot)
				throw std::runtime_error("Duplicate root directive");
			if (i + 1 >= tokens.size())
				throw std::runtime_error("Missing argument for 'root'");
			server.setRoot(tokens[i + 1]);
			hasRoot = true;
			i += 2;
		}
		else if (token == "error_page")
		{
			if (i + 2 >= tokens.size())
				std::runtime_error("Missing argument for 'error_page'");
			int	code;
			std::stringstream ss(tokens[i + 1]);
			ss >> code;
			if (ss.fail())
				throw std::runtime_error("Invalid error code for 'error_page'");
			std::string	path = tokens[i + 2];
			server.setErrorPage(code, path);
			i += 3;
		}
		else if (token == "index")
		{
			if (hasIndex)
				throw std::runtime_error("Duplicate index directive");
			if (i + 1 >= tokens.size())
				throw std::runtime_error("Missing argument for 'index'");
			server.setIndexFile(tokens[i + 1]);
			hasIndex = true;
			i += 2;
		}
		else if (token == "client_max_body_size")
		{
			if (hasBodySize)
				throw std::runtime_error("Duplicate client_max_body_size directive");
			if (i + 1 >= tokens.size())
				throw std::runtime_error("Missing argument for 'client_max_body_size'");
			parseClientBodySize(tokens[i + 1], server);
			hasBodySize = true;
			i += 2;
		}
		else if (token == "autoindex")
		{
			if (hasAutoIndex)
				throw std::runtime_error("Duplicate autoindex directive");
			if (i + 1 >= tokens.size())
				throw std::runtime_error("Missing argument for 'autoindex'");
			std::string flag = tokens[i + 1];
			if (flag == "on")
				server.setAutoindex(true);
			else if (flag == "off")
				server.setAutoindex(false);
			else
				throw std::runtime_error("Invalid value for autoindex: must be 'on' or 'off'");
			hasAutoIndex = true;
			i += 2;
		}
		else if (token == "location")
		{
			parseLocationBlock(tokens, i, server);
			hasLocation = true;
			continue ;
		}
		else
			throw std::runtime_error("Unknown directive in server block: " + token);
		expect(tokens, i, ";");
	}
	if (!hasRoot)
		throw std::runtime_error("Missing root directive");
	if (!hasLocation)
		throw std::runtime_error("Missing location directive");
	config.addServer(server);
}

std::vector<std::string>	ConfigParser::tokenize(std::istringstream& in)
{
	std::string					word;
	std::vector<std::string>	tokens;

	while (in >> word)
	{
		std::string	current;

		for (std::size_t i = 0; i < word.size(); i++)
		{
			char	c = word[i];

			if (c == '{' || c == '}' || c == ';')
			{
				if (!current.empty())
				{
					tokens.push_back(current);
					current.clear();
				}
				std::string	metachar(1, c); //saving {, } or ;
				tokens.push_back(metachar);
			}
			else
				current += c;
		}
		if (!current.empty())
			tokens.push_back(current);
	}
	return (tokens);
}

std::string	ConfigParser::cleanConfigFile(std::ifstream& file)
{
	std::ostringstream	clean;
	std::string			line;

	while (std::getline(file, line))
	{
		for (std::string::iterator it = line.begin(); it != line.end(); ++it)
			*it = std::tolower(*it);
		std::string::size_type	commentPos = line.find('#');
		if (commentPos != std::string::npos)
			line.erase(commentPos); //erase comments
		if (!line.empty())
			clean << line << '\n';
	}
	file.close();
	return (clean.str());
}

void	debug_print(std::vector<std::string> tokens)
{
	for (std::size_t i = 0; i < tokens.size(); i++)
		std::cout << i << ": [" << tokens[i] << "]" << std::endl;
}

Config	ConfigParser::parseFile(std::string const& configFile)
{
	std::ifstream	rawFile(configFile.c_str());
	if (!rawFile.is_open())
		throw std::runtime_error("Filed to open config file: " + configFile);

	std::string					cleaned = cleanConfigFile(rawFile);
	std::istringstream			file(cleaned);
	std::vector<std::string>	tokens = tokenize(file);
	//debug_print(tokens); //debug
	std::size_t					i = 0;
	Config						config;

	while (i < tokens.size())
	{
		if (tokens[i] == "server")
			parseServerBlock(tokens, i, config);
		else
			throw std::runtime_error("Unknown directive: " + tokens[i]);
		expect(tokens, i, "}");
	}
	return (config);
}