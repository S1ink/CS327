#include "new/game.hpp"
#include "util/debug.hpp"

#include <atomic>

#include <signal.h>


static std::atomic<bool> is_running{ true };
static void handle_exit(int x)
{
    is_running = false;
}
static inline void init_sig()
{
    signal(SIGINT, handle_exit);
    signal(SIGQUIT, handle_exit);
    signal(SIGILL, handle_exit);
    signal(SIGABRT, handle_exit);
    signal(SIGBUS, handle_exit);
    signal(SIGFPE, handle_exit);
    signal(SIGSEGV, handle_exit);
    signal(SIGTERM, handle_exit);
    signal(SIGSTKFLT, handle_exit);
    signal(SIGXCPU, handle_exit);
    signal(SIGXFSZ, handle_exit);
    signal(SIGPWR, handle_exit);
}

inline int main_108(int argc, char** argv)
{
    init_sig();
    GameApplication g{ argc, argv, is_running };
    g.run();

    return 0;
}


int main(int argc, char** argv)
{
    return main_108(argc, argv);
}
