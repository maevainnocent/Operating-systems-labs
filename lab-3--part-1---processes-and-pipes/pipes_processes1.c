#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

/*
 * pipes_processes1.c
 * Two-way communication between parent (P1) and child (P2)
 *
 * Flow:
 *  1. Parent sends input to child.
 *  2. Child concatenates "howard.edu" and prints it.
 *  3. Child asks for another input from user and appends it.
 *  4. Child sends the new string back to parent.
 *  5. Parent appends "gobison.org" and prints final result.
 *
 * Example:
 *   Input: www.geeks
 *   Output: www.geekshoward.edu
 *   Input: helloworld
 *   Output: www.geekshoward.eduhelloworldgobison.org
 */

int main() {
    int fd1[2]; // Pipe 1: P1 -> P2
    int fd2[2]; // Pipe 2: P2 -> P1
    pid_t p;

    char fixed_str1[] = "howard.edu";
    char fixed_str2[] = "gobison.org";
    char input_str[100], concat_str[200], second_input[100];

    if (pipe(fd1) == -1 || pipe(fd2) == -1) {
        fprintf(stderr, "Pipe Failed\n");
        return 1;
    }

    p = fork();

    if (p < 0) {
        fprintf(stderr, "Fork Failed\n");
        return 1;
    }

    // -------------------- Parent Process --------------------
    if (p > 0) {
        close(fd1[0]);  // Close reading end of pipe1
        close(fd2[1]);  // Close writing end of pipe2

        printf("Other string is: howard.edu\n");
        printf("Input: ");
        scanf("%s", input_str);

        // Write input string to child
        write(fd1[1], input_str, strlen(input_str) + 1);

        // Wait for child to send concatenated result back
        read(fd2[0], concat_str, sizeof(concat_str));

        // Append "gobison.org" and print final result
        strcat(concat_str, fixed_str2);
        printf("Output: %s\n", concat_str);

        close(fd1[1]);
        close(fd2[0]);
        wait(NULL);
    }

    // -------------------- Child Process --------------------
    else {
        close(fd1[1]);  // Close writing end of pipe1
        close(fd2[0]);  // Close reading end of pipe2

        // Read string from parent
        read(fd1[0], concat_str, sizeof(concat_str));

        // Concatenate "howard.edu"
        strcat(concat_str, fixed_str1);
        printf("Output: %s\n", concat_str);

        // Ask for another input
        printf("Input: ");
        scanf("%s", second_input);

        // Append second input to the string
        strcat(concat_str, second_input);

        // Send final string back to parent
        write(fd2[1], concat_str, strlen(concat_str) + 1);

        close(fd1[0]);
        close(fd2[1]);
        exit(0);
    }

    return 0;
}
