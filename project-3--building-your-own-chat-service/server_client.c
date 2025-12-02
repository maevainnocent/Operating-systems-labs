/* server_client.c */
#include "server.h"
#include "list.h"

extern int numReaders;
extern pthread_mutex_t mutex;
extern pthread_mutex_t rw_lock;

extern struct node *head;         // user list
extern struct room_node *room_head; // room list

const char *delimiters_local = delimiters; // from server.h

/* trim whitespace helper */
char *trimwhitespace(char *str)
{
  char *end;
  while(isspace((unsigned char)*str)) str++;
  if(*str == 0)  return str;
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;
  end[1] = '\0';
  return str;
}

/* Reader-writer wrappers */
void start_read() {
    pthread_mutex_lock(&mutex);
    numReaders++;
    if (numReaders == 1) pthread_mutex_lock(&rw_lock);
    pthread_mutex_unlock(&mutex);
}
void end_read() {
    pthread_mutex_lock(&mutex);
    numReaders--;
    if (numReaders == 0) pthread_mutex_unlock(&rw_lock);
    pthread_mutex_unlock(&mutex);
}
void start_write() {
    pthread_mutex_lock(&rw_lock);
}
void end_write() {
    pthread_mutex_unlock(&rw_lock);
}

/* Safe send wrapper */
ssize_t safe_send(int socket, const char *buf) {
    if (socket <= 0 || buf == NULL) return -1;
    return send(socket, buf, strlen(buf), 0);
}

/* Check if two users share a room */
int share_room_users(struct node *a, struct node *b) {
    if (!a || !b) return 0;
    struct room_node *r = room_head;
    while (r) {
        struct room_member *m = r->members;
        int hasa = 0, hasb = 0;
        while (m) {
            if (m->user_sock == a->socket) hasa = 1;
            if (m->user_sock == b->socket) hasb = 1;
            m = m->next;
        }
        if (hasa && hasb) return 1;
        r = r->next;
    }
    return 0;
}

/* Main per-client thread */
void *client_receive(void *ptr) {
   int client = *(int *) ptr;
   free(ptr);

   int received;
   char buffer[MAXBUFF], sbuffer[MAXBUFF];
   char tmpbuf[MAXBUFF];
   char cmd[MAXBUFF];
   char *arguments[80];

   // send MOTD
   safe_send(client, server_MOTD);

   // create guest username and insert
   char username[64];
   snprintf(username, sizeof(username), "guest%d", client);

   start_write();
   head = insertFirstU(head, client, username); // original list.c function
   // ensure default room exists and add user to it
   room_head = create_room(room_head, DEFAULT_ROOM);
   add_user_to_room(&room_head, client, DEFAULT_ROOM);
   end_write();

   while (1) {
      memset(buffer, 0, sizeof(buffer));
      received = recv(client, buffer, MAXBUFF-1, 0);
      if (received <= 0) {
         // client disconnected — cleanup
         start_write();
         struct node *u = findU(head, username);
         if (u) {
             remove_user_from_all_rooms(&room_head, u);
             remove_all_dms_for_user(head, u);
             head = removeUserBySocket(head, client);
         }
         end_write();
         close(client);
         break;
      }
      buffer[received] = '\0';
      strcpy(cmd, buffer);
      strcpy(sbuffer, buffer);

      // Tokenize (split by server.h delimiters macro; here single space)
      arguments[0] = strtok(cmd, delimiters_local);
      int i = 0;
      while (arguments[i] != NULL) {
         arguments[++i] = strtok(NULL, delimiters_local);
         if (arguments[i-1]) strcpy(arguments[i-1], trimwhitespace(arguments[i-1]));
      }

      if (!arguments[0]) {
         safe_send(client, "\nchat>");
         continue;
      }

      /* ---------- COMMANDS ---------- */
      if (strcmp(arguments[0], "create") == 0 && arguments[1]) {
         start_write();
         room_head = create_room(room_head, arguments[1]);
         end_write();
         snprintf(buffer, sizeof(buffer), "Room '%s' created\nchat>", arguments[1]);
         safe_send(client, buffer);
      }
      else if (strcmp(arguments[0], "join") == 0 && arguments[1]) {
         start_write();
         add_user_to_room(&room_head, client, arguments[1]);
         end_write();
         snprintf(buffer, sizeof(buffer), "Joined room '%s'\nchat>", arguments[1]);
         safe_send(client, buffer);
      }
      else if (strcmp(arguments[0], "leave") == 0 && arguments[1]) {
         start_write();
         int r = remove_user_from_room(&room_head, client, arguments[1]);
         end_write();
         if (r == 0) snprintf(buffer, sizeof(buffer), "Left room '%s'\nchat>", arguments[1]);
         else snprintf(buffer, sizeof(buffer), "Not a member of room '%s'\nchat>", arguments[1]);
         safe_send(client, buffer);
      }
      else if (strcmp(arguments[0], "connect") == 0 && arguments[1]) {
         start_write();
         struct node *to = findU(head, arguments[1]);
         struct node *from = findSocketNode(head, client);
         if (!to) {
             end_write();
             snprintf(buffer, sizeof(buffer), "User '%s' not found\nchat>", arguments[1]);
             safe_send(client, buffer);
         } else {
             add_dm_connection_socket(head, from->socket, to->socket);
             end_write();
             snprintf(buffer, sizeof(buffer), "Connected to user '%s'\nchat>", arguments[1]);
             safe_send(client, buffer);
         }
      }
      else if (strcmp(arguments[0], "disconnect") == 0 && arguments[1]) {
         start_write();
         struct node *to = findU(head, arguments[1]);
         struct node *from = findSocketNode(head, client);
         if (!to) {
             end_write();
             snprintf(buffer, sizeof(buffer), "User '%s' not found\nchat>", arguments[1]);
             safe_send(client, buffer);
         } else {
             remove_dm_connection_socket(head, from->socket, to->socket);
             end_write();
             snprintf(buffer, sizeof(buffer), "Disconnected from user '%s'\nchat>", arguments[1]);
             safe_send(client, buffer);
         }
      }
      else if (strcmp(arguments[0], "rooms") == 0) {
         start_read();
         char out[4096]; out[0]='\0';
         list_rooms_to_buffer(room_head, out, sizeof(out));
         end_read();
         strncat(out, "chat>", sizeof(out)-strlen(out)-1);
         safe_send(client, out);
      }
      else if (strcmp(arguments[0], "users") == 0) {
         start_read();
         char out[4096]; out[0]='\0';
         list_users_to_buffer(head, out, sizeof(out));
         end_read();
         strncat(out, "chat>", sizeof(out)-strlen(out)-1);
         safe_send(client, out);
      }
      else if (strcmp(arguments[0], "login") == 0 && arguments[1]) {
         start_write();
         struct node *u = findSocketNode(head, client);
         if (u) {
             strncpy(u->username, arguments[1], sizeof(u->username)-1);
             u->username[sizeof(u->username)-1] = '\0';
         }
         end_write();
         snprintf(buffer, sizeof(buffer), "Logged in as '%s'\nchat>", arguments[1]);
         safe_send(client, buffer);
      }
      else if (strcmp(arguments[0], "help") == 0) {
         snprintf(buffer, sizeof(buffer),
             "login <username> - \"login with username\" \ncreate <room> - \"create a room\" \njoin <room> - \"join a room\" \nleave <room> - \"leave a room\" \nusers - \"list all users\" \nrooms -  \"list all rooms\" \nconnect <user> - \"connect to user\" \nexit - \"exit chat\"\nchat>");
         safe_send(client, buffer);
      }
      else if (strcmp(arguments[0], "exit") == 0 || strcmp(arguments[0], "logout") == 0) {
         start_write();
         struct node *u = findSocketNode(head, client);
         if (u) {
             remove_user_from_all_rooms(&room_head, u);
             remove_all_dms_for_user(head, u);
             head = removeUserBySocket(head, client);
         }
         end_write();
         close(client);
         break;
      }
      else {
         /* Not a command — broadcast to shared-room members and DMs */
         start_read();
         struct node *sender = findSocketNode(head, client);
         char sendbuf[MAXBUFF+128];
         char trimmed[MAXBUFF];
         strncpy(trimmed, trimwhitespace(sbuffer), sizeof(trimmed)-1);
         trimmed[sizeof(trimmed)-1] = '\0';
         if (sender) snprintf(sendbuf, sizeof(sendbuf), "\n::%s> %s\nchat>", sender->username, trimmed);
         else snprintf(sendbuf, sizeof(sendbuf), "\n::guest%d> %s\nchat>", client, trimmed);

         struct node *iter = head;
         while (iter) {
             if (iter->socket != client) {
                 int sendit = 0;
                 // DM check
                 if (is_dm_connected_socket(head, client, iter->socket)) sendit = 1;
                 // share room check
                 else if (share_room_users(sender, iter)) sendit = 1;
                 if (sendit) safe_send(iter->socket, sendbuf);
             }
             iter = iter->next;
         }
         end_read();
      }
   } // while

   return NULL;
}
