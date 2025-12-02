#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

int seconds = 0;

void alarm_handler(int signum) {
    seconds++;
    alarm(1); // trigger again in 1 second
}

void int_handler(int signum) {
    printf("\nProgram executed for %d seconds.\n", seconds);
    exit(0);
}

int main() {
    signal(SIGALRM, alarm_handler);
    signal(SIGINT, int_handler);

    alarm(1); // trigger first alarm

    while (1)
        pause(); // wait for next signal
}
