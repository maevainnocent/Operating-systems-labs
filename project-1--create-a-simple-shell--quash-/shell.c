#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#define MAX_COMMAND_LINE_LEN 1024
#define MAX_COMMAND_LINE_ARGS 128
#define MAX_PIPES 16

char prompt[MAX_COMMAND_LINE_LEN];
char delimiters[] = " \t\r\n";
extern char **environ;

pid_t foreground_pid = -1;

// SIGINT handler
void sigint_handler(int sig) {
    if (foreground_pid > 0) {
        kill(foreground_pid, SIGINT);
    }
    printf("\n");
    fflush(stdout);
}

// SIGALRM handler (Task 5)
void sigalrm_handler(int sig) {
    if (foreground_pid > 0) {
        kill(foreground_pid, SIGKILL);
        printf("\n[Foreground process killed after 10s]\n");
        fflush(stdout);
    }
}

// Tokenize input
int tokenize(char *command_line, char **arguments) {
    int argc = 0;
    char *token = strtok(command_line, delimiters);
    while (token != NULL && argc < MAX_COMMAND_LINE_ARGS - 1) {
        if (token[0] == '$') {
            char *value = getenv(token + 1);
            arguments[argc++] = value ? strdup(value) : strdup("");
        } else {
            arguments[argc++] = strdup(token);
        }
        token = strtok(NULL, delimiters);
    }
    arguments[argc] = NULL;
    return argc;
}

// Handle built-ins
bool handle_builtin(char **arguments, int argc) {
    if (argc == 0) return false;
    if (strcmp(arguments[0], "exit") == 0) exit(0);
    if (strcmp(arguments[0], "cd") == 0) {
        if (argc < 2) fprintf(stderr, "cd: missing argument\n");
        else if (chdir(arguments[1]) != 0) perror("cd");
        return true;
    }
    if (strcmp(arguments[0], "pwd") == 0) {
        char cwd[MAX_COMMAND_LINE_LEN];
        if (getcwd(cwd, sizeof(cwd)) != NULL) printf("%s\n", cwd);
        else perror("getcwd");
        return true;
    }
    if (strcmp(arguments[0], "echo") == 0) {
        for (int i = 1; i < argc; i++) printf("%s ", arguments[i]);
        printf("\n");
        return true;
    }
    if (strcmp(arguments[0], "env") == 0) {
        if (argc == 1) for (char **env = environ; *env != 0; env++) printf("%s\n", *env);
        else { char *val = getenv(arguments[1]); if (val) printf("%s\n", val); }
        return true;
    }
    if (strcmp(arguments[0], "setenv") == 0) {
        if (argc < 3) fprintf(stderr, "setenv: missing arguments\n");
        else if (setenv(arguments[1], arguments[2], 1) != 0) perror("setenv");
        return true;
    }
    return false;
}

// Execute a single command with optional I/O redirection
void execute_command(char **arguments) {
    // Handle I/O redirection
    for (int i = 0; arguments[i] != NULL; i++) {
        if (strcmp(arguments[i], ">") == 0) {
            arguments[i] = NULL;
            int fd = open(arguments[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) { perror("open"); exit(1); }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        } else if (strcmp(arguments[i], ">>") == 0) {
            arguments[i] = NULL;
            int fd = open(arguments[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd < 0) { perror("open"); exit(1); }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        } else if (strcmp(arguments[i], "<") == 0) {
            arguments[i] = NULL;
            int fd = open(arguments[i + 1], O_RDONLY);
            if (fd < 0) { perror("open"); exit(1); }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
    }
    execvp(arguments[0], arguments);
    perror("execvp failed");
    exit(1);
}

// Multi-pipe execution
void execute_pipeline(char **commands[], int n) {
    int pipefd[2 * (n - 1)];
    for (int i = 0; i < n - 1; i++) pipe(pipefd + i * 2);

    pid_t pids[n];
    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid == 0) { // child
            if (i > 0) { // read from previous pipe
                dup2(pipefd[(i - 1) * 2], STDIN_FILENO);
            }
            if (i < n - 1) { // write to next pipe
                dup2(pipefd[i * 2 + 1], STDOUT_FILENO);
            }
            for (int j = 0; j < 2 * (n - 1); j++) close(pipefd[j]);
            execute_command(commands[i]);
        } else {
            pids[i] = pid;
        }
    }
    for (int j = 0; j < 2 * (n - 1); j++) close(pipefd[j]);
    for (int i = 0; i < n; i++) waitpid(pids[i], NULL, 0);
}

int main() {
    signal(SIGINT, sigint_handler);
    signal(SIGALRM, sigalrm_handler);

    char command_line[MAX_COMMAND_LINE_LEN];
    char *arguments[MAX_COMMAND_LINE_ARGS];

    while (true) {
        if (getcwd(prompt, sizeof(prompt)) != NULL) {
            printf("%s> ", prompt);
            fflush(stdout);
        }

        if ((fgets(command_line, sizeof(command_line), stdin) == NULL)) {
            printf("\n"); break;
        }
        if (command_line[0] == '\n') continue;
        command_line[strcspn(command_line, "\n")] = '\0';

        // Split by pipes
        char *cmds[MAX_PIPES];
        int n_cmds = 0;
        char *tok = strtok(command_line, "|");
        while (tok && n_cmds < MAX_PIPES) cmds[n_cmds++] = tok, tok = strtok(NULL, "|");

        if (n_cmds == 1) {
            int argc = tokenize(cmds[0], arguments);
            if (argc == 0) continue;
            bool background = false;
            if (strcmp(arguments[argc - 1], "&") == 0) {
                background = true;
                arguments[argc - 1] = NULL;
            }

            if (handle_builtin(arguments, argc)) continue;

            foreground_pid = fork();
            if (foreground_pid == 0) { // child
                execute_command(arguments);
            } else if (foreground_pid > 0) {
                if (!background) {
                    alarm(10); // set 10-second timer
                    waitpid(foreground_pid, NULL, 0);
                    alarm(0);  // cancel timer
                } else {
                    printf("[Background pid %d]\n", foreground_pid);
                }
                foreground_pid = -1;
            } else {
                perror("fork failed");
            }
        } else { // multiple pipes
            char **cmd_args[MAX_PIPES];
            for (int i = 0; i < n_cmds; i++) {
                cmd_args[i] = malloc(sizeof(char*) * MAX_COMMAND_LINE_ARGS);
                tokenize(cmds[i], cmd_args[i]);
            }
            execute_pipeline(cmd_args, n_cmds);
            for (int i = 0; i < n_cmds; i++) free(cmd_args[i]);
        }
    }
    return 0;
}
