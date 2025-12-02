#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

int signal_received = 0;

void handler(int signum) {
    printf("Hello World!\n");
    signal_received = 1;
    alarm(5); // re-arm alarm
}

int main() {
    signal(SIGALRM, handler);
    alarm(5); // start initial alarm

    while (1) {
        if (signal_received) {
            printf("Turing was right!\n");
            signal_received = 0;
        }
        pause(); // sleep until next signal
    }
}
