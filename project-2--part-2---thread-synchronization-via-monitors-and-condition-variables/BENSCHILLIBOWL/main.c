#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

#include "BENSCHILLIBOWL.h"

// Feel free to play with these numbers! This is a great way to
// test your implementation.
#define BENSCHILLIBOWL_SIZE 100
#define NUM_CUSTOMERS 90
#define NUM_COOKS 10
#define ORDERS_PER_CUSTOMER 3
#define EXPECTED_NUM_ORDERS (NUM_CUSTOMERS * ORDERS_PER_CUSTOMER)

// Global variable for the restaurant.
BENSCHILLIBOWL *bcb;

/**
 * Thread funtion that represents a customer. A customer should:
 *  - allocate space (memory) for an order.
 *  - select a menu item.
 *  - populate the order with their menu item and their customer ID.
 *  - add their order to the restaurant.
 */
void* BENSCHILLIBOWLCustomer(void* tid) {
    int customer_id = (int)(long) tid;
    // each customer places ORDERS_PER_CUSTOMER orders
    for (int i = 0; i < ORDERS_PER_CUSTOMER; ++i) {
        // allocate order
        Order *order = malloc(sizeof(Order));
        if (!order) {
            perror("malloc order");
            continue;
        }

        // pick menu item and populate fields
        order->menu_item = PickRandomMenuItem();
        order->customer_id = customer_id;
        order->order_number = 0;
        order->next = NULL;

        int order_num = AddOrder(bcb, order);
        printf("Customer #%d placed order %d (%s)\n", customer_id, order_num, order->menu_item);

        // random short sleep to emulate real life
        usleep((rand() % 200) * 1000); // 0 - 199 ms
    }
    return NULL;
}

/**
 * Thread function that represents a cook in the restaurant. A cook should:
 *  - get an order from the restaurant.
 *  - if the order is valid, it should fulfill the order, and then
 *    free the space taken by the order.
 * The cook should take orders from the restaurants until it does not
 * receive an order.
 */
void* BENSCHILLIBOWLCook(void* tid) {
    int cook_id = (int)(long) tid;
    int orders_fulfilled = 0;

    while (1) {
        Order *order = GetOrder(bcb);
        if (order == NULL) {
            // no more orders
            break;
        }

        // Fulfill the order (simulate time)
        usleep((rand() % 500 + 100) * 1000); // 100ms - 599ms

        // update handled count under lock
        pthread_mutex_lock(&bcb->mutex);
        bcb->orders_handled++;
        // notify CloseRestaurant possibly waiting on orders_handled
        if (bcb->orders_handled >= bcb->expected_num_orders) {
            pthread_cond_broadcast(&bcb->can_get_orders);
            pthread_cond_broadcast(&bcb->can_add_orders);
        } else {
            // also signal can_add_orders so customers blocking can continue
            pthread_cond_signal(&bcb->can_add_orders);
        }
        pthread_mutex_unlock(&bcb->mutex);

        printf("Cook #%d fulfilled order %d for Customer #%d (%s). Total handled: %d\n",
               cook_id, order->order_number, order->customer_id, order->menu_item, bcb->orders_handled);

        // free order structure
        free(order);

        orders_fulfilled++;
    }

    printf("Cook #%d fulfilled %d orders\n", cook_id, orders_fulfilled);
    return NULL;
}

/**
 * Runs when the program begins executing. This program should:
 *  - open the restaurant
 *  - create customers and cooks
 *  - wait for all customers and cooks to be done
 *  - close the restaurant.
 */
int main() {
    srand(time(NULL) ^ getpid());

    bcb = OpenRestaurant(BENSCHILLIBOWL_SIZE, EXPECTED_NUM_ORDERS);
    if (!bcb) {
        fprintf(stderr, "Failed to open restaurant\n");
        return 1;
    }

    pthread_t customers[NUM_CUSTOMERS];
    pthread_t cooks[NUM_COOKS];

    // create cook threads
    for (long i = 0; i < NUM_COOKS; ++i) {
        if (pthread_create(&cooks[i], NULL, BENSCHILLIBOWLCook, (void*)i) != 0) {
            perror("pthread_create cook");
            return 1;
        }
    }

    // create customer threads
    for (long i = 0; i < NUM_CUSTOMERS; ++i) {
        if (pthread_create(&customers[i], NULL, BENSCHILLIBOWLCustomer, (void*)i) != 0) {
            perror("pthread_create customer");
            return 1;
        }
    }

    // wait for customers to finish (they place all orders)
    for (int i = 0; i < NUM_CUSTOMERS; ++i) {
        pthread_join(customers[i], NULL);
    }

    // At this point all orders have been placed. Cooks will keep consuming until GetOrder returns NULL.
    // Wait for all cooks to finish.
    for (int i = 0; i < NUM_COOKS; ++i) {
        pthread_join(cooks[i], NULL);
    }

    // All cooks finished - close restaurant
    CloseRestaurant(bcb);

    return 0;
}
