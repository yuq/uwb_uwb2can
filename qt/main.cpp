#include <unistd.h>
#include <signal.h>
#include <wait.h>
#include <iostream>

int server_main(int argc, char **argv);

static pid_t pid;
static bool exit_loop = true;

static void signal_handler(int signo)
{
    if (signo == SIGINT)
        std::cout << "main receive signal: SIGINT\n";
    if (signo == SIGTERM)
        std::cout << "main receive signal: SIGTERM\n";

    if (kill(pid, SIGTERM) < 0)
        std::cerr << "main kill server fail\n";

    exit_loop = false;
}

int main(int argc, char **argv)
{
    if (signal(SIGINT, signal_handler) == SIG_ERR ||
        signal(SIGTERM, signal_handler) == SIG_ERR) {
        std::cerr << "main process register signal handler fail\n";
        return 0;
    }

    do {
        std::cout << "main start fork server\n";
        pid = fork();
        if (pid == 0) {
            if (signal(SIGINT, SIG_DFL) == SIG_ERR ||
                signal(SIGTERM, SIG_DFL) == SIG_ERR) {
                std::cerr << "server process register signal handler fail\n";
                return 0;
            }
            return server_main(argc, argv);
        }
        else if (pid > 0) {
            int status;
            wait(&status);
            std::cout << "server exit with status: " << status << std::endl;
        }
        else {
            std::cerr << "main fork server fail: " << pid << std::endl;
            break;
        }
    } while (exit_loop);

    return 0;
}


