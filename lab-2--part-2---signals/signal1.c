#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

int signal_received = 0; // global flag

void handler(int signum) {
    printf("Hello World!\n");
    signal_received = 1; // signal caught
}

int main() {
    signal(SIGALRM, handler);
    alarm(5); // deliver signal after 5 seconds

    while (!signal_received); // wait until signal received

    printf("Turing was right!\n");
    return 0;
}
