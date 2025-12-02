#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include "list.h"

#define TRUE 1
#define FALSE 0
#define PORT 8888
#define BACKLOG 2
#define MAXBUFF 2096
#define MAX_CLIENTS 30
#define DEFAULT_ROOM "Lobby"
#define DELIMITERS " "

/* Struct passed to each client thread */
typedef struct client_info {
    int sockfd;
    struct sockaddr_in addr;
} client_info;

/* Server socket setup */
int get_server_socket();
int start_server(int serv_socket, int backlog);
int accept_client(int serv_sock);

/* Thread worker */
void *client_receive(void *ptr);

/* Signal handling */
void sigintHandler(int sig_num);

#endif
