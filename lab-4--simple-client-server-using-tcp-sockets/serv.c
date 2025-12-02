#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include "list.h"

#define PORT 9001
#define ACK "ACK"

int serverSocket = -1;
int clientSocket = -1;
list_t *mylist = NULL;

// Graceful exit handler
void handle_exit(int sig) {
    if (mylist) list_free(mylist);
    if (clientSocket != -1) close(clientSocket);
    if (serverSocket != -1) close(serverSocket);
    printf("\nServer exiting gracefully...\n");
    exit(0);
}

int main(int argc, char const* argv[]) {
    signal(SIGINT, handle_exit); // Catch Ctrl-C

    int n, val, idx;
    char recvBuf[1024];
    char sendBuf[1024];
    char *token;

    // Create server socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    // Define server address
    struct sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(PORT);
    servAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket
    if (bind(serverSocket, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
        perror("Bind failed");
        close(serverSocket);
        exit(1);
    }

    // Listen for connections
    if (listen(serverSocket, 1) < 0) {
        perror("Listen failed");
        close(serverSocket);
        exit(1);
    }

    printf("Server is running on port %d...\n", PORT);

    // Accept a client connection
    clientSocket = accept(serverSocket, NULL, NULL);
    if (clientSocket < 0) {
        perror("Accept failed");
        close(serverSocket);
        exit(1);
    }

    // Allocate linked list
    mylist = list_alloc();

    while (1) {
        n = recv(clientSocket, recvBuf, sizeof(recvBuf) - 1, 0);
        if (n <= 0) {
            printf("Client disconnected.\n");
            close(clientSocket);

            // Wait for a new client
            clientSocket = accept(serverSocket, NULL, NULL);
            if (clientSocket < 0) {
                perror("Accept failed");
                handle_exit(0);
            }
            continue;
        }

        recvBuf[n] = '\0';
        token = strtok(recvBuf, " ");

        if (strcmp(token, "exit") == 0) {
            handle_exit(0);
        }
        else if (strcmp(token, "get_length") == 0) {
            val = list_length(mylist);
            sprintf(sendBuf, "Length = %d", val);
        }
        else if (strcmp(token, "add_front") == 0) {
            token = strtok(NULL, " ");
            val = atoi(token);
            list_add_to_front(mylist, val);
            sprintf(sendBuf, "%s %d", ACK, val);
        }
        else if (strcmp(token, "add_back") == 0) {
            token = strtok(NULL, " ");
            val = atoi(token);
            list_add_to_back(mylist, val);
            sprintf(sendBuf, "%s %d", ACK, val);
        }
        else if (strcmp(token, "add_position") == 0) {
            token = strtok(NULL, " ");
            idx = atoi(token);
            token = strtok(NULL, " ");
            val = atoi(token);
            list_add_at_index(mylist, idx, val);
            sprintf(sendBuf, "%s %d at %d", ACK, val, idx);
        }
        else if (strcmp(token, "remove_front") == 0) {
            val = list_remove_from_front(mylist);
            sprintf(sendBuf, "Removed %d from front", val);
        }
        else if (strcmp(token, "remove_back") == 0) {
            val = list_remove_from_back(mylist);
            sprintf(sendBuf, "Removed %d from back", val);
        }
        else if (strcmp(token, "remove_position") == 0) {
            token = strtok(NULL, " ");
            idx = atoi(token);
            val = list_remove_at_index(mylist, idx);
            sprintf(sendBuf, "Removed %d at %d", val, idx);
        }
        else if (strcmp(token, "get") == 0) {
            token = strtok(NULL, " ");
            idx = atoi(token);
            val = list_get_elem_at(mylist, idx);
            sprintf(sendBuf, "Value at %d = %d", idx, val);
        }
        else if (strcmp(token, "print") == 0) {
            sprintf(sendBuf, "%s", listToString(mylist));
        }
        else {
            sprintf(sendBuf, "Invalid command");
        }

        send(clientSocket, sendBuf, strlen(sendBuf), 0); // send actual string length
        memset(recvBuf, '\0', sizeof(recvBuf));
        memset(sendBuf, '\0', sizeof(sendBuf));
    }

    return 0;
}
