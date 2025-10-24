#include <utils/signals.hpp>

volatile std::sig_atomic_t Signals::g_shouldStop = 0;

void Signals::signalHandle(int signal)
{
    (void)signal;
    std::cout << red << "\n Exiting program " << reset << std::endl;
    g_shouldStop = 1;
}

bool Signals::shouldStop()
{
    return g_shouldStop != 0;
}