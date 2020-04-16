#include <signal.h>
#include <unistd.h>

void signal_handler(int signo);

int main() {
    signal(SIGINT, signal_handler);
    sleep(5);
    return 0;
}

void signal_handler(int signo) {
    write(STDOUT_FILENO, "enter\n", 6);
    if (signo == SIGINT) {
        int count = 0;
        while (count < 2) {
            write(STDOUT_FILENO, "check\n", 6);
        count++;
        }
    }
}