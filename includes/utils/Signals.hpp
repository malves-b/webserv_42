#ifndef SIGNALS_HPP
# define SIGNALS_HPP

#include <csignal>
#include <ctime>
#include <map>
#include <unistd.h>

class Signals
{
	private:
		static volatile std::sig_atomic_t	g_shouldStop;
		
	public:
		Signals(void);
		~Signals(void);
		static const int					CGI_TIMEOUT_SEC = 30;
		
		static void	signalHandle(int signal);
		static void	childSignalHandle(int signal);

		static bool	shouldStop(void);
		static void	setupHandlers(void);

		static void	registerCgiProcess(pid_t pid);
		static void	unregisterCgiProcess(pid_t pid);
		static bool	hasActiveCgi(void);
		static void	checkCgiTimeouts(void);
};

#endif
