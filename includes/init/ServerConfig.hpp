#ifndef SERVER_CONFIG_HPP
# define SERVER_CONFIG_HPP

#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

//webservinho
#include <request/RequestMethod.hpp>

//Just only for test
class ServerConfig
{
	private:
		ServerConfig();
		~ServerConfig();

	public:
		static ServerConfig&	instance();

		const std::string			root;
		const std::string			index;
		const std::string			cgiPath;
		const std::string			error_page_404; //TODO
		std::vector<RequestMethod::Method>	allow_methods; //TODO
		std::size_t					client_max_body_size;
		//TODO return
		bool						autoindex; //TODO matheus
		const std::string			upload_path;  //TODO
};

#endif //SERVER_CONFIG_HPP