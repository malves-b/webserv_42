#include <utils/signals.hpp>
#include <utils/Logger.hpp>

volatile std::sig_atomic_t Signals::g_shouldStop = 0;

void Signals::signalHandle(int signal)
{
    (void)signal;
	Logger::instance().log(INFO, "Exiting program");
    g_shouldStop = 1;
	Logger::instance().~Logger();
}

bool Signals::shouldStop()
{
    return g_shouldStop != 0;
}