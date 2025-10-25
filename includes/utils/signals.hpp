#if !defined SIGNALS_HPP
#define SIGNALS_HPP

#include <iostream>
#include <csignal>
#include <cstdlib>

#define red "\033[31m"
#define reset "\033[0m"

class Signals
{
private:
    static volatile std::sig_atomic_t g_shouldStop;

public:
    static void signalHandle(int signal);
    static bool shouldStop();
};

#endif