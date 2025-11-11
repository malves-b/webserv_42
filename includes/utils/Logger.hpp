#ifndef LOGGER_HPP
# define LOGGER_HPP

#ifndef DEV
# define DEV 0
#endif //DEV

#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>

enum LogLevel
{
	DEBUG,
	INFO,
	WARNING,
	ERROR,
	CRITICAL
};

class Logger
{
	private:
		std::ofstream		_logFile;
		const std::string	levelToString(LogLevel level) const;
		Logger();

	public:
		~Logger();
		static Logger&	instance();
		void			log(LogLevel level, const std::string& message);
};

#endif //LOGGER_HPP