#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

/*
 * pipes_processes3.c
 * This program chains three processes together using two pipes:
 * 
 * cat scores | grep <argument> | sort
 * 
 * Example:
 *   ./pipes_proc3 28
 * behaves like:
 *   cat scores | grep 28 | sort
 */

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <search_term>\n", argv[0]);
        exit(1);
    }

    int pipe1[2];  // Between cat and grep
    int pipe2[2];  // Between grep and sort

    pipe(pipe1);
    pipe(pipe2);

    pid_t pid1 = fork();

    if (pid1 == 0) {
        // ----------- CHILD 1: executes "cat scores" -----------
        dup2(pipe1[1], STDOUT_FILENO); // redirect stdout to pipe1 write end
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);

        char *cat_args[] = {"cat", "scores", NULL};
        execvp("cat", cat_args);
        perror("execvp cat failed");
        exit(1);
    }

    pid_t pid2 = fork();

    if (pid2 == 0) {
        // ----------- CHILD 2: executes "grep <argument>" -----------
        dup2(pipe1[0], STDIN_FILENO);  // read from pipe1
        dup2(pipe2[1], STDOUT_FILENO); // write to pipe2
        close(pipe1[1]);
        close(pipe1[0]);
        close(pipe2[0]);
        close(pipe2[1]);

        char *grep_args[] = {"grep", argv[1], NULL};
        execvp("grep", grep_args);
        perror("execvp grep failed");
        exit(1);
    }

    pid_t pid3 = fork();

    if (pid3 == 0) {
        // ----------- CHILD 3: executes "sort" -----------
        dup2(pipe2[0], STDIN_FILENO); // read from pipe2
        close(pipe2[1]);
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);

        char *sort_args[] = {"sort", NULL};
        execvp("sort", sort_args);
        perror("execvp sort failed");
        exit(1);
    }

    // ----------- PARENT closes all pipe ends -----------
    close(pipe1[0]);
    close(pipe1[1]);
    close(pipe2[0]);
    close(pipe2[1]);

    // Wait for all children to finish
    wait(NULL);
    wait(NULL);
    wait(NULL);

    return 0;
}
