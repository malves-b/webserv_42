#include <utils/Signals.hpp>
#include <utils/Logger.hpp>
#include <utils/string_utils.hpp>
#include <sys/wait.h>
#include <unistd.h>
#include <ctime>
#include <map>

volatile std::sig_atomic_t Signals::g_shouldStop = 0;
static std::map<pid_t, std::time_t> g_activeCgis;

/**
 * @brief Default constructor for Signals utility class.
 */
Signals::Signals(void) {}

/**
 * @brief Destructor for Signals.
 */
Signals::~Signals(void) {}

/**
 * @brief Handles SIGINT and SIGTERM for graceful shutdown.
 *
 * Sets a global flag (`g_shouldStop`) to notify the main loop that
 * the server should stop safely. Writes directly to stderr to ensure
 * visibility even if logging fails.
 */
void	Signals::signalHandle(int signal)
{
	(void)signal;
	g_shouldStop = 1;
	write(STDERR_FILENO, "\n[Signal] Graceful shutdown requested\n", 38);
	Logger::instance().log(INFO, "SIGINT received: graceful shutdown requested");
}

/**
 * @brief Handles SIGCHLD signals to clean up terminated child processes.
 *
 * Uses non-blocking `waitpid()` calls with `WNOHANG` to reap CGI processes
 * and remove their PIDs from the active map.
 */
void	Signals::childSignalHandle(int signal)
{
	(void)signal;
	int		status;
	pid_t	pid;

	while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
		unregisterCgiProcess(pid);
}

/**
 * @brief Registers a new active CGI process.
 *
 * @param pid Process ID of the CGI script.
 * Adds the PID to an internal map with its start timestamp.
 */
void	Signals::registerCgiProcess(pid_t pid)
{
	g_activeCgis[pid] = std::time(NULL);
	Logger::instance().log(DEBUG, "Signals: registered CGI pid=" + toString(pid));
}

/**
 * @brief Unregisters a finished CGI process.
 *
 * Removes its entry from the internal active CGI map.
 */
void	Signals::unregisterCgiProcess(pid_t pid)
{
	std::map<pid_t, std::time_t>::iterator it = g_activeCgis.find(pid);
	if (it != g_activeCgis.end())
	{
		Logger::instance().log(DEBUG, "Signals: unregistered CGI pid=" + toString(pid));
		g_activeCgis.erase(it);
	}
}

/**
 * @brief Checks if there are currently active CGI processes.
 *
 * @return true if one or more CGIs are running, false otherwise.
 */
bool	Signals::hasActiveCgi(void)
{
	return (!g_activeCgis.empty());
}

/**
 * @brief Periodically checks for CGI timeouts and terminates those exceeding the limit.
 *
 * Any CGI process that runs longer than `CGI_TIMEOUT_SEC` seconds is forcefully killed.
 * Also removes its PID from the active CGI map.
 */
void	Signals::checkCgiTimeouts(void)
{
	std::time_t now = std::time(NULL);

	for (std::map<pid_t, std::time_t>::iterator it = g_activeCgis.begin(); it != g_activeCgis.end(); )
	{
		double elapsed = difftime(now, it->second);
		if (elapsed > Signals::CGI_TIMEOUT_SEC)
		{
			Logger::instance().log(WARNING,
				"Signals: killing CGI pid=" + toString(it->first) +
				" (timeout " + toString(Signals::CGI_TIMEOUT_SEC) + "s exceeded)");
			kill(it->first, SIGKILL);

			std::map<pid_t, std::time_t>::iterator toErase = it;
			++it;
			g_activeCgis.erase(toErase);
		}
		else
			++it;
	}
}

/**
 * @brief Checks whether a stop signal was received.
 *
 * @return true if the shutdown flag is set, false otherwise.
 */
bool	Signals::shouldStop(void)
{
	return (g_shouldStop != 0);
}

/**
 * @brief Registers signal handlers for SIGINT, SIGTERM, and SIGCHLD.
 *
 * Enables graceful shutdown and automatic cleanup of CGI child processes.
 */
void	Signals::setupHandlers(void)
{
	std::signal(SIGINT, Signals::signalHandle);
	std::signal(SIGTERM, Signals::signalHandle);
	std::signal(SIGCHLD, Signals::childSignalHandle);
	Logger::instance().log(INFO, "Signal handlers registered");
}
