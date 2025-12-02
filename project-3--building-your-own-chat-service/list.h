#ifndef LIST_H
#define LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/* ----------------------------
   User list node (used elsewhere as struct node)
   ---------------------------- */
struct dm_node {
    int socket;               // socket of the DM peer
    struct dm_node *next;
};

struct node {
   char username[30];
   int socket;
   struct dm_node *dm;       // linked list of DM peers (by socket)
   struct node *next;
};

/* ----------------------------
   Room list and members
   ---------------------------- */
struct room_member {
    int user_sock;               // socket of user in room
    struct room_member *next;
};

struct room_node {
    char roomname[50];
    struct room_member *members;
    struct room_node *next;
};

/* ----------------------------
   Globals (defined in list.c)
   ---------------------------- */
extern struct node *head;           // global user list head
extern struct room_node *room_head; // global room list head

/* ----------------------------
   User list functions (original API)
   ---------------------------- */
struct node* insertFirstU(struct node *head, int socket, char *username);
struct node* findU(struct node *head, char* username);
struct node* findSocketNode(struct node *head, int socket);
struct node* removeUserBySocket(struct node *head, int socket);
void free_all_users(struct node *head);

/* ----------------------------
   Room functions
   ---------------------------- */
struct room_node* create_room(struct room_node *head, const char *roomname);
struct room_node* find_room(struct room_node *head, const char *roomname);
void free_all_rooms(struct room_node *head);
int add_user_to_room(struct room_node **head, int socket, const char *roomname);
int remove_user_from_room(struct room_node **head, int socket, const char *roomname);
void list_rooms_to_buffer(struct room_node *head, char *buf, size_t buflen);
void list_users_to_buffer(struct node *head, char *buf, size_t buflen);

/* ----------------------------
   DM management (by socket)
   ---------------------------- */
int add_dm_connection_socket(struct node *head, int from_sock, int to_sock);
int remove_dm_connection_socket(struct node *head, int from_sock, int to_sock);
int is_dm_connected_socket(struct node *head, int from_sock, int to_sock);

/* ----------------------------
   Utilities
   ---------------------------- */
void remove_user_from_all_rooms(struct room_node **head, struct node *user);
void remove_all_dms_for_user(struct node *head, struct node *user);

#endif
