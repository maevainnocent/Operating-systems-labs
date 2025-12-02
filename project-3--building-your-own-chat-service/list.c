#include "list.h"

/* Global heads */
struct node *head = NULL;
struct room_node *room_head = NULL;

/* ----------------------------
   Users (original functions)
   ---------------------------- */

struct node* insertFirstU(struct node *head_local, int socket, char *username) {
   if(findU(head_local,username) == NULL) {
       struct node *link = (struct node*) malloc(sizeof(struct node));
       if (!link) return head_local;
       link->socket = socket;
       strncpy(link->username, username, sizeof(link->username)-1);
       link->username[sizeof(link->username)-1] = '\0';
       link->dm = NULL;
       link->next = head_local;
       head_local = link;
   } else {
       // duplicate username -- we keep original behavior: do not insert duplicate name
   }
   return head_local;
}

struct node* findU(struct node *head_local, char* username) {
   struct node* current = head_local;
   if(head_local == NULL) return NULL;
   while(current) {
      if(strcmp(current->username, username) == 0) return current;
      current = current->next;
   }
   return NULL;
}

struct node* findSocketNode(struct node *head_local, int socket) {
    struct node *cur = head_local;
    while (cur) {
        if (cur->socket == socket) return cur;
        cur = cur->next;
    }
    return NULL;
}

struct node* removeUserBySocket(struct node *head_local, int socket) {
    struct node *cur = head_local, *prev = NULL;
    while (cur) {
        if (cur->socket == socket) {
            if (prev) prev->next = cur->next;
            else head_local = cur->next;
            // free dm list
            struct dm_node *d = cur->dm;
            while (d) {
                struct dm_node *dt = d->next;
                free(d);
                d = dt;
            }
            free(cur);
            break;
        }
        prev = cur;
        cur = cur->next;
    }
    return head_local;
}

void free_all_users(struct node *head_local) {
    struct node *cur = head_local;
    while (cur) {
        struct node *tmp = cur->next;
        struct dm_node *d = cur->dm;
        while (d) {
            struct dm_node *dt = d->next;
            free(d);
            d = dt;
        }
        free(cur);
        cur = tmp;
    }
}

/* ----------------------------
   Rooms and members
   ---------------------------- */

struct room_node* create_room(struct room_node *head_r, const char *roomname) {
    if (find_room(head_r, roomname) != NULL) return head_r;
    struct room_node *r = malloc(sizeof(struct room_node));
    if (!r) return head_r;
    strncpy(r->roomname, roomname, sizeof(r->roomname)-1);
    r->roomname[sizeof(r->roomname)-1] = '\0';
    r->members = NULL;
    r->next = head_r;
    head_r = r;
    return head_r;
}

struct room_node* find_room(struct room_node *head_r, const char *roomname) {
    struct room_node *cur = head_r;
    while (cur) {
        if (strcmp(cur->roomname, roomname) == 0) return cur;
        cur = cur->next;
    }
    return NULL;
}

void free_all_rooms(struct room_node *head_r) {
    struct room_node *cur = head_r;
    while (cur) {
        struct room_member *m = cur->members;
        while (m) {
            struct room_member *mt = m->next;
            free(m);
            m = mt;
        }
        struct room_node *tmp = cur->next;
        free(cur);
        cur = tmp;
    }
}

/* Add user (socket) to room. If room doesn't exist, create it. */
int add_user_to_room(struct room_node **head_r_ptr, int socket, const char *roomname) {
    struct room_node *r = find_room(*head_r_ptr, roomname);
    if (!r) {
        *head_r_ptr = create_room(*head_r_ptr, roomname);
        r = find_room(*head_r_ptr, roomname);
        if (!r) return -1;
    }
    // check if already present
    struct room_member *m = r->members;
    while (m) {
        if (m->user_sock == socket) return 0;
        m = m->next;
    }
    struct room_member *nm = malloc(sizeof(struct room_member));
    if (!nm) return -1;
    nm->user_sock = socket;
    nm->next = r->members;
    r->members = nm;
    return 0;
}

/* Remove user from a named room */
int remove_user_from_room(struct room_node **head_r_ptr, int socket, const char *roomname) {
    struct room_node *r = find_room(*head_r_ptr, roomname);
    if (!r) return -1;
    struct room_member *cur = r->members, *prev = NULL;
    while (cur) {
        if (cur->user_sock == socket) {
            if (prev) prev->next = cur->next;
            else r->members = cur->next;
            free(cur);
            return 0;
        }
        prev = cur;
        cur = cur->next;
    }
    return -1;
}

/* List rooms into buffer */
void list_rooms_to_buffer(struct room_node *head_r, char *buf, size_t buflen) {
    buf[0] = '\0';
    struct room_node *cur = head_r;
    while (cur) {
        strncat(buf, cur->roomname, buflen - strlen(buf) - 2);
        strncat(buf, "\n", buflen - strlen(buf) - 1);
        cur = cur->next;
    }
}

/* List users into buffer */
void list_users_to_buffer(struct node *head_local, char *buf, size_t buflen) {
    buf[0] = '\0';
    struct node *cur = head_local;
    while (cur) {
        strncat(buf, cur->username, buflen - strlen(buf) - 2);
        char tmp[64];
        snprintf(tmp, sizeof(tmp), " (socket %d)\n", cur->socket);
        strncat(buf, tmp, buflen - strlen(buf) - 1);
        cur = cur->next;
    }
}

/* ----------------------------
   DM management (by socket)
   ---------------------------- */

int add_dm_connection_socket(struct node *head_local, int from_sock, int to_sock) {
    if (!findSocketNode(head_local, from_sock) || !findSocketNode(head_local, to_sock)) return -1;
    // Add to from's dm list if not present
    struct node *from = findSocketNode(head_local, from_sock);
    struct dm_node *d = from->dm;
    while (d) {
        if (d->socket == to_sock) return 0;
        d = d->next;
    }
    struct dm_node *nd = malloc(sizeof(struct dm_node));
    if (!nd) return -1;
    nd->socket = to_sock;
    nd->next = from->dm;
    from->dm = nd;
    // Add reverse
    struct node *to = findSocketNode(head_local, to_sock);
    struct dm_node *nd2 = malloc(sizeof(struct dm_node));
    if (!nd2) return -1;
    nd2->socket = from_sock;
    nd2->next = to->dm;
    to->dm = nd2;
    return 0;
}

int remove_dm_connection_socket(struct node *head_local, int from_sock, int to_sock) {
    struct node *from = findSocketNode(head_local, from_sock);
    struct node *to = findSocketNode(head_local, to_sock);
    if (!from || !to) return -1;
    // remove from's entry
    struct dm_node *cur = from->dm, *prev = NULL;
    while (cur) {
        if (cur->socket == to_sock) {
            if (prev) prev->next = cur->next;
            else from->dm = cur->next;
            free(cur);
            break;
        }
        prev = cur; cur = cur->next;
    }
    // remove reverse
    cur = to->dm; prev = NULL;
    while (cur) {
        if (cur->socket == from_sock) {
            if (prev) prev->next = cur->next;
            else to->dm = cur->next;
            free(cur);
            break;
        }
        prev = cur; cur = cur->next;
    }
    return 0;
}

int is_dm_connected_socket(struct node *head_local, int from_sock, int to_sock) {
    struct node *from = findSocketNode(head_local, from_sock);
    if (!from) return 0;
    struct dm_node *d = from->dm;
    while (d) {
        if (d->socket == to_sock) return 1;
        d = d->next;
    }
    return 0;
}

/* ----------------------------
   Utilities: remove user from all rooms
   ---------------------------- */

void remove_user_from_all_rooms(struct room_node **head_r_ptr, struct node *user) {
    if (!user) return;
    struct room_node *r = *head_r_ptr;
    while (r) {
        remove_user_from_room(head_r_ptr, user->socket, r->roomname);
        r = r->next;
    }
}

/* Remove all DM entries referencing this user */
void remove_all_dms_for_user(struct node *head_local, struct node *user) {
    struct node *cur = head_local;
    while (cur) {
        struct dm_node *dcur = cur->dm, *dprev = NULL;
        while (dcur) {
            if (dcur->socket == user->socket) {
                if (dprev) dprev->next = dcur->next;
                else cur->dm = dcur->next;
                free(dcur);
                break;
            }
            dprev = dcur;
            dcur = dcur->next;
        }
        cur = cur->next;
    }
}

