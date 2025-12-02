/* server.c */
#include "server.h"
#include "list.h"

int chat_serv_sock_fd; // server socket

/////////////////////////////////////////////
// USE THESE LOCKS AND COUNTER TO SYNCHRONIZE

int numReaders = 0; // keep count of the number of readers

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  // mutex lock for reader count
pthread_mutex_t rw_lock = PTHREAD_MUTEX_INITIALIZER;  // read/write lock

/////////////////////////////////////////////

char const *server_MOTD = "Thanks for connecting to the BisonChat Server.\n\nchat>";

/* Global lists (defined in list.c) */
extern struct node *head;     // user list (list.c uses 'struct node' per your original)
extern struct room_node *room_head; // room list (we add room structures)

void init_default_room();
void free_all_global_resources();

void init_default_room() {
    // create Lobby at startup
    pthread_mutex_lock(&rw_lock);
    room_head = create_room(room_head, DEFAULT_ROOM);
    pthread_mutex_unlock(&rw_lock);
}

int main(int argc, char **argv) {
   signal(SIGINT, sigintHandler);

   init_default_room();

   // Open server socket
   chat_serv_sock_fd = get_server_socket();

   // step 3: get ready to accept connections
   if(start_server(chat_serv_sock_fd, BACKLOG) == -1) {
      printf("start server error\n");
      exit(1);
   }

   printf("Server Launched! Listening on PORT: %d\n", PORT);

   //Main execution loop
   while(1) {
      //Accept a connection, start a thread
      int *pclient = malloc(sizeof(int));
      if (!pclient) continue;
      *pclient = accept_client(chat_serv_sock_fd);
      if(*pclient != -1) {
         pthread_t new_client_thread;
         pthread_create(&new_client_thread, NULL, client_receive, (void *)pclient);
         pthread_detach(new_client_thread);
      } else {
         free(pclient);
      }
   }

   close(chat_serv_sock_fd);
   return 0;
}

/* returns a listening server socket bound to PORT */
int get_server_socket() {
    int opt = TRUE;
    int master_socket;
    struct sockaddr_in address;

    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );

    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    return master_socket;
}

int start_server(int serv_socket, int backlog) {
   int status = 0;
   if ((status = listen(serv_socket, backlog)) == -1) {
      printf("socket listen error\n");
   }
   return status;
}

int accept_client(int serv_sock) {
   int reply_sock_fd = -1;
   socklen_t sin_size = sizeof(struct sockaddr_storage);
   struct sockaddr_storage client_addr;

   if ((reply_sock_fd = accept(serv_sock,(struct sockaddr *)&client_addr, &sin_size)) == -1) {
      printf("socket accept error\n");
   }
   return reply_sock_fd;
}

/* Handle SIGINT (CTRL+C) - graceful shutdown */
void sigintHandler(int sig_num) {
   (void)sig_num;
   printf("\nSIGINT received. Shutting down server gracefully...\n");

   pthread_mutex_lock(&rw_lock); // block writers/readers

   // Close all client sockets and free users
   struct node *cur = head;
   while (cur) {
       printf("Closing socket for user %s (socket %d)\n", cur->username, cur->socket);
       close(cur->socket);
       cur = cur->next;
   }

   // free data structures
   free_all_rooms(room_head);
   room_head = NULL;

   free_all_users(head);
   head = NULL;

   pthread_mutex_unlock(&rw_lock);

   // close server socket
   close(chat_serv_sock_fd);
   printf("Server shutdown complete.\n");
   exit(0);
}
