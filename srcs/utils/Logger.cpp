#include <utils/Logger.hpp>

/**
 * @brief Initializes the Logger and opens a daily log file.
 *
 * Creates (or appends to) a log file named as:
 * `./logs/webserv_YYYY-MM-DD.log`
 */
Logger::Logger()
{
	std::time_t now = time(0);
	tm* timeinfo = localtime(&now);
	char timestamp[20];
	std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d", timeinfo);

	std::string	filename = "./logs/webserv_";
	filename += timestamp;
	filename += ".log";

	_logFile.open(filename.c_str(), std::ios::app);
	if (!_logFile.is_open())
	{
		std::cerr << "Error opening log file." << std::endl;
	}
}

/**
 * @brief Closes the active log file on destruction.
 */
Logger::~Logger() { _logFile.close(); }

/**
 * @brief Returns the singleton instance of the Logger.
 *
 * Ensures that only one instance of Logger exists throughout execution.
 */
Logger&	Logger::instance()
{
	static Logger logger;
	return (logger);
}

/**
 * @brief Writes a log entry with timestamp, level, and message.
 *
 * Outputs to console depending on DEV mode:
 * - `DEV = 0`: only INFO and higher.
 * - `DEV = 1`: all levels.
 * Always writes to the log file if open.
 */
void	Logger::log(LogLevel level, const std::string& message)
{
	std::time_t now = time(0);
	tm* timeinfo = localtime(&now);
	char timestamp[20];
	std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);

	std::ostringstream logEntry;

	logEntry << "[" << timestamp << "] "
				<< levelToString(level) << ": " << message
				<< std::endl;

	// Console output
	if (DEV == 0 && level >= INFO)
		std::cout << logEntry.str();
	else if (DEV == 1)
		std::cout << logEntry.str();

	// File output
	if (_logFile.is_open())
	{
		_logFile << logEntry.str();
		_logFile.flush();
	}
}

/**
 * @brief Converts a LogLevel enum value to its string label.
 *
 * @param level The log severity level.
 * @return Corresponding string ("DEBUG", "INFO", etc.).
 */
const std::string	Logger::levelToString(LogLevel level) const
{
	switch (level)
	{
		case DEBUG:
			return "DEBUG";
		case INFO:
			return "INFO";
		case WARNING:
			return "WARNING";
		case ERROR:
			return "ERROR";
		case CRITICAL:
			return "CRITICAL";
		default:
			return "UNKNOWN";
	}
}
