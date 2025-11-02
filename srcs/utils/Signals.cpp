#include <utils/Signals.hpp>
#include <utils/Logger.hpp>
#include <utils/string_utils.hpp>
#include <sys/wait.h>
#include <unistd.h>
#include <ctime>
#include <map>

volatile std::sig_atomic_t Signals::g_shouldStop = 0;
static std::map<pid_t, std::time_t> g_activeCgis;
static const int CGI_TIMEOUT_SEC = 30;

Signals::Signals(void) {}

Signals::~Signals(void) {}

void	Signals::signalHandle(int signal)
{
	(void)signal;
	g_shouldStop = 1;
	write(STDERR_FILENO, "\n[Signal] Graceful shutdown requested\n", 38);
	Logger::instance().log(INFO, "SIGINT received: graceful shutdown requested");
}

void	Signals::childSignalHandle(int signal)
{
	(void)signal;
	int		status;
	pid_t	pid;

	while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
		unregisterCgiProcess(pid);
}

void	Signals::registerCgiProcess(pid_t pid)
{
	g_activeCgis[pid] = std::time(NULL);
	Logger::instance().log(DEBUG, "Signals: registered CGI pid=" + toString(pid));
}

void	Signals::unregisterCgiProcess(pid_t pid)
{
	std::map<pid_t, std::time_t>::iterator it = g_activeCgis.find(pid);
	if (it != g_activeCgis.end())
	{
		Logger::instance().log(DEBUG, "Signals: unregistered CGI pid=" + toString(pid));
		g_activeCgis.erase(it);
	}
}

bool	Signals::hasActiveCgi(void)
{
	return (!g_activeCgis.empty());
}

void	Signals::checkCgiTimeouts(void)
{
	std::time_t now = std::time(NULL);

	for (std::map<pid_t, std::time_t>::iterator it = g_activeCgis.begin(); it != g_activeCgis.end(); )
	{
		double elapsed = difftime(now, it->second);
		if (elapsed > CGI_TIMEOUT_SEC)
		{
			Logger::instance().log(WARNING,
				"Signals: killing CGI pid=" + toString(it->first) +
				" (timeout " + toString(CGI_TIMEOUT_SEC) + "s exceeded)");
			kill(it->first, SIGKILL);

			std::map<pid_t, std::time_t>::iterator toErase = it;
			++it;
			g_activeCgis.erase(toErase);
		}
		else
			++it;
	}
}

bool	Signals::shouldStop(void)
{
	return (g_shouldStop != 0);
}

void	Signals::setupHandlers(void)
{
	std::signal(SIGINT, Signals::signalHandle);
	std::signal(SIGTERM, Signals::signalHandle);
	std::signal(SIGCHLD, Signals::childSignalHandle);
	Logger::instance().log(INFO, "Signal handlers registered");
}
