#include <iostream>
#include <string>
#include <csignal>
#include <init/WebServer.hpp>
#include <init/ServerSocket.hpp>
#include <config/ConfigParser.hpp>
#include <config/Config.hpp>
#include <utils/Logger.hpp>
#include <utils/Signals.hpp>

int main(int argc, char** argv)
{
	Logger::instance();
	Logger::instance().log(INFO, "[Started] Webservinho");

	std::signal(SIGINT, Signals::signalHandle);

	std::string configFile;

	if (argc > 2)
	{
		std::cerr << "Usage: ./webserv [config_file]" << std::endl;
		return (1);
	}
	if (argc == 1)
	{
		configFile = "default.conf";
		Logger::instance().log(WARNING, "No config file specified. Using default.conf");
	}
	else if (argc == 2)
	{
		configFile = argv[1];
	}

	try
	{
		Logger::instance().log(INFO, "Parsing configuration: " + configFile);

		Config config = ConfigParser::parseFile(configFile);
		WebServer server(config);

		Logger::instance().log(INFO, "Starting server...");
		server.startServer();

		Logger::instance().log(INFO, "Running main loop...");
		server.runServer();
	}
	catch (const std::exception& e)
	{
		Logger::instance().log(ERROR, std::string("Fatal: ") + e.what());
		std::cerr << "Fatal: " << e.what() << std::endl;
		return (1);
	}

	Logger::instance().log(INFO, "[Finished] Webservinho");
	return (0);
}
