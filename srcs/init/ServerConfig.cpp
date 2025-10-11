#include <init/ServerConfig.hpp>

ServerConfig::ServerConfig() : root("/home/malves-b/sgoinfre/webserv/webservinho_app"),
	index("index.html"),
	cgiPath("/home/malves-b/sgoinfre/webserv/webservinho_app/cgi-bin"),
	error_page_404("errors/404.html"),
	client_max_body_size(1048576),
	autoindex(true),
	upload_path("/home/malves-b/sgoinfre/webserv/webservinho_app/uploads")
{}

ServerConfig::~ServerConfig() {}

ServerConfig&	ServerConfig::instance()
{
	static ServerConfig serverConfig;
	return (serverConfig);
}
