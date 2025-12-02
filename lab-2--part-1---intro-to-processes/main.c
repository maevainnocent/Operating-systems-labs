#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

void child_process(int n) {
    for (int i = 0; i < n; i++) {
        printf("Child PID: %d, Parent PID: %d, i=%d\n", getpid(), getppid(), i);
        sleep(1);
    }
}

int main() {
    printf("Parent PID: %d\n", getpid());

    pid_t pid1 = fork();
    if (pid1 == 0) {
        child_process(5);
        exit(0);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        child_process(5);
        exit(0);
    }

    // Parent waits for children
    wait(NULL);
    wait(NULL);

    printf("Parent process %d finished.\n", getpid());
    return 0;
}