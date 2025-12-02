#include "BENSCHILLIBOWL.h"

#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

bool IsEmpty(BENSCHILLIBOWL* bcb);
bool IsFull(BENSCHILLIBOWL* bcb);
void AddOrderToBack(Order **orders, Order *order);

MenuItem BENSCHILLIBOWLMenu[] = { 
    "BensChilli", 
    "BensHalfSmoke", 
    "BensHotDog", 
    "BensChilliCheeseFries", 
    "BensShake",
    "BensHotCakes",
    "BensCake",
    "BensHamburger",
    "BensVeggieBurger",
    "BensOnionRings",
};
int BENSCHILLIBOWLMenuLength = 10;

/* Select a random item from the Menu and return it */
MenuItem PickRandomMenuItem() {
    int idx = rand() % BENSCHILLIBOWLMenuLength;
    return BENSCHILLIBOWLMenu[idx];
}

/* Allocate memory for the Restaurant, then create the mutex and condition variables needed to instantiate the Restaurant */

BENSCHILLIBOWL* OpenRestaurant(int max_size, int expected_num_orders) {
    BENSCHILLIBOWL* bcb = malloc(sizeof(BENSCHILLIBOWL));
    if (!bcb) {
        perror("malloc");
        return NULL;
    }

    bcb->orders = NULL;
    bcb->current_size = 0;
    bcb->max_size = max_size;
    bcb->next_order_number = 1;
    bcb->orders_handled = 0;
    bcb->expected_num_orders = expected_num_orders;

    if (pthread_mutex_init(&bcb->mutex, NULL) != 0) {
        perror("pthread_mutex_init");
        free(bcb);
        return NULL;
    }
    if (pthread_cond_init(&bcb->can_add_orders, NULL) != 0) {
        perror("pthread_cond_init can_add_orders");
        pthread_mutex_destroy(&bcb->mutex);
        free(bcb);
        return NULL;
    }
    if (pthread_cond_init(&bcb->can_get_orders, NULL) != 0) {
        perror("pthread_cond_init can_get_orders");
        pthread_cond_destroy(&bcb->can_add_orders);
        pthread_mutex_destroy(&bcb->mutex);
        free(bcb);
        return NULL;
    }

    printf("Restaurant is open!\n");
    return bcb;
}


/* check that the number of orders received is equal to the number handled (ie.fullfilled). Remember to deallocate your resources */

void CloseRestaurant(BENSCHILLIBOWL* bcb) {
    if (!bcb) return;

    // Wait until all orders are handled
    pthread_mutex_lock(&bcb->mutex);
    while (bcb->orders_handled < bcb->expected_num_orders) {
        pthread_cond_wait(&bcb->can_get_orders, &bcb->mutex);
    }
    pthread_mutex_unlock(&bcb->mutex);

    // Destroy synchronization objects
    pthread_mutex_destroy(&bcb->mutex);
    pthread_cond_destroy(&bcb->can_add_orders);
    pthread_cond_destroy(&bcb->can_get_orders);

    // Free any remaining orders (shouldn't be any)
    Order *cur = bcb->orders;
    while (cur) {
        Order *tmp = cur->next;
        free(cur);
        cur = tmp;
    }

    free(bcb);
    printf("Restaurant is closed!\n");
}

/* add an order to the back of queue */
int AddOrder(BENSCHILLIBOWL* bcb, Order* order) {
    if (!bcb || !order) return -1;

    pthread_mutex_lock(&bcb->mutex);

    // Wait until not full
    while (IsFull(bcb)) {
        pthread_cond_wait(&bcb->can_add_orders, &bcb->mutex);
    }

    // assign order number
    order->order_number = bcb->next_order_number++;
    order->next = NULL;

    // add to back of queue
    AddOrderToBack(&bcb->orders, order);
    bcb->current_size++;

    // Signal to cooks that an order is available
    pthread_cond_signal(&bcb->can_get_orders);

    pthread_mutex_unlock(&bcb->mutex);

    return order->order_number;
}

/* remove an order from the queue */
Order *GetOrder(BENSCHILLIBOWL* bcb) {
    if (!bcb) return NULL;

    pthread_mutex_lock(&bcb->mutex);

    // Wait while empty AND not all orders received/handled
    while (IsEmpty(bcb)) {
        // If we've received all expected orders (next_order_number - 1)
        int orders_received = bcb->next_order_number - 1;
        if (orders_received >= bcb->expected_num_orders) {
            // No more orders will be added; notify other cooks and return NULL
            pthread_cond_broadcast(&bcb->can_get_orders);
            pthread_mutex_unlock(&bcb->mutex);
            return NULL;
        }
        // Otherwise wait for new orders
        pthread_cond_wait(&bcb->can_get_orders, &bcb->mutex);
    }

    // There is at least one order in queue
    Order *order = bcb->orders;
    bcb->orders = order->next;
    order->next = NULL;
    bcb->current_size--;

    pthread_mutex_unlock(&bcb->mutex);

    // Signal producers (customers) that there is space
    pthread_cond_signal(&bcb->can_add_orders);

    return order;
}

// Optional helper functions (you can implement if you think they would be useful)
bool IsEmpty(BENSCHILLIBOWL* bcb) {
  return (bcb->orders == NULL);
}

bool IsFull(BENSCHILLIBOWL* bcb) {
  return (bcb->current_size >= bcb->max_size);
}

/* this methods adds order to rear of queue */
void AddOrderToBack(Order **orders, Order *order) {
    if (*orders == NULL) {
        *orders = order;
    } else {
        Order *cur = *orders;
        while (cur->next != NULL) cur = cur->next;
        cur->next = order;
    }
}
