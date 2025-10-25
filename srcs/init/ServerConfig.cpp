#include <config/ServerConfig.hpp>

ServerConfig::ServerConfig() : root("/home/malves-b/sgoinfre/webserv/webservinho_app"),
	index("index.html"),
	cgiPath("/home/jose/webserv_42/webservinho_app/cgi-bin"),
	client_max_body_size(1048576),
	autoindex(true)
{
	redirect = "/static/about.html";
	upload_path = "/home/jose/webserv_42/webservinho_app/uploads";
	this->allow_methods.push_back(RequestMethod::GET);
	this->allow_methods.push_back(RequestMethod::POST);
	error_page[404] = "webservinho_app/errors/404.html";
	error_page[403] = "webservinho_app/errors/403.html";
}

ServerConfig::~ServerConfig() {}

ServerConfig&	ServerConfig::instance()
{
	static ServerConfig serverConfig;
	return (serverConfig);
}

const std::string	ServerConfig::errorPage(int error)
{
	std::map<int, std::string>::const_iterator c_it;

	c_it = this->error_page.find(error);
	if (c_it != this->error_page.end())
		return (c_it->second);
	return ("");
}
