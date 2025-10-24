#include <init/WebServer.hpp>
#include <init/ServerSocket.hpp>
#include <init/ServerConfig.hpp>
#include <utils/Logger.hpp>
#include <iostream>
#include <string>
#include <utils/signals.hpp>/*NEW*/


int	main(int argc, char** argv)
{
	Logger::instance();
	Logger::instance().log(INFO, "Webservinho Started");

	ServerConfig::instance();
	std::string	configFile;

	std::signal(SIGINT, Signals::signalHandle); /* NEW*/

	if (argc > 2)
	{
		std::cout << "usage: ./webserv [config_file]" << std::endl;
		return (1);
	}
	if (argc == 1)
		configFile = "defautFile"; //create file and put it in the repository?
	if (argc == 2)
		configFile = argv[1];
	try
	{

		WebServer	server;
		server.startServer();
		server.runServer();
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
	return (0);
}
