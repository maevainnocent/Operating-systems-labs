#include <stdio.h>      // For printf()
#include <stdlib.h>     // For exit(), srandom(), random()
#include <unistd.h>     // For fork(), sleep(), getpid(), getppid()
#include <sys/wait.h>   // For wait()

void child_process();

int main() {
    pid_t pid;
    int i;

    // Create 2 child processes
    for (i = 0; i < 2; i++) {
        pid = fork();

        if (pid < 0) {
            // Fork failed
            perror("Fork failed");
            exit(1);
        } 
        else if (pid == 0) {
            // Child process
            child_process();
            exit(0); // Should never return to parent’s loop
        }
        // Parent continues loop to create next child
    }

    // Parent waits for both child processes to finish
    for (i = 0; i < 2; i++) {
        int status;
        pid_t completed_pid = wait(&status);
        printf("Child Pid: %d has completed with exit status: %d\n", completed_pid, status);
    }

    printf("Parent process %d: All children have completed.\n", getpid());
    return 0;
}

// ----------------------
// Child Process Function
// ----------------------
void child_process() {
    pid_t pid = getpid();
    pid_t parent_pid = getppid();

    // Seed random number generator uniquely for each child
    srandom(pid);

    // Random number of iterations (1–30)
    int loops = 1 + (random() % 30);

    for (int i = 0; i < loops; i++) {
        int sleep_time = 1 + (random() % 10); // random sleep 1–10 seconds
        printf("Child Pid: %d is going to sleep for %d seconds!\n", pid, sleep_time);
        sleep(sleep_time);
        printf("Child Pid: %d is awake!\nWhere is my Parent: %d?\n", pid, parent_pid);
    }

    printf("Child Pid: %d is done!\n", pid);
}
