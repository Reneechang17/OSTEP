/**
 * OSTEP - Concurrency
 * 
 * Thread-safe linked list
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define NUM_THREADS 4
#define OPERATIONS_PER_THREAD 1000
// #define OPERATIONS_PER_THREAD 100000

// Node for linked list
typedef struct node
{
    int key;
    struct node *next;
} node_t;

// Thread-safe linked list
typedef struct  
{
    node_t *head;
    pthread_mutex_t lock;
} list_t;

void list_init(list_t *list) {
    list->head = NULL;
    pthread_mutex_init(&list->lock, NULL);
}

// Insert a key
// For simplifed, we insert at the head
int list_insert(list_t *list, int key) {
    // Allocate the new node outside the critical section
    // This is safe because malloc is thread-safe
    node_t *new_node = (node_t *)malloc(sizeof(node_t));
    if (new_node == NULL) {
        return -1; // Allocation failed
    }
    new_node->key = key;

    // Only lock for the actual list update
    pthread_mutex_lock(&list->lock);
    new_node->next = list->head;  // Critical section
    list->head = new_node;        // Critical section
    pthread_mutex_unlock(&list->lock);

    return 0;
}

// Lookup a ket in the list
int list_lookup(list_t *list, int key) {
    int found = 0;

    pthread_mutex_lock(&list->lock);
    node_t *cur = list->head;
    while (cur != NULL) {
        if (cur->key == key) {
            found = 1;
            break;
        }
        cur = cur->next;
    }
    pthread_mutex_unlock(&list->lock);

    return found;
}

// Count elements
int list_count(list_t *list) {
    int cnt = 0;

    pthread_mutex_lock(&list->lock);
    node_t *cur = list->head;
    while (cur != NULL) {
        cnt++;
        cur = cur->next;
    }
    pthread_mutex_unlock(&list->lock);

    return cnt;
}

// Cleanup list
void list_destroy(list_t *list) {
    pthread_mutex_lock(&list->lock);

    node_t *cur = list->head;
    while (cur != NULL) {
        node_t *tmp = cur;
        cur = cur->next;
        free(tmp);
    }
    list->head = NULL;

    pthread_mutex_unlock(&list->lock);
    pthread_mutex_destroy(&list->lock);
}

// Print list
void list_print(list_t *list, int max_items) {
    pthread_mutex_lock(&list->lock);

    printf("List contents (first %d items): ", max_items);
    node_t *cur = list->head;
    int cnt = 0;
    while (cur != NULL && cnt < max_items) {
        printf("%d", cur->key);
        cur = cur->next;
        cnt++;
    }
    if (cur != NULL) {
        printf("...");
    }
    printf("\n");

    pthread_mutex_unlock(&list->lock);
}

// Thread arg structure
typedef struct  
{
    list_t *list;
    int thread_id;
    int start_val;
    int num_ops;
} thread_arg_t;

void* thread_ops(void* arg) {
    thread_arg_t *targ = (thread_arg_t *)arg;

    // Each thread inserts its own range of values
    for (int i = 0; i < targ->num_ops; i++) {
        int val = targ->start_val + i;
        // int val = i;

        if (list_insert(targ->list, val) == 0) {
            if (i % 10 == 0) {
                int found = list_lookup(targ->list, val);
                if (!found) {
                    printf("Thread %d: ERROR - just inserted %d but can't find it\n", targ->thread_id, val);
                }
            }
        }
    }
    printf("Thread %d: Completed %d insertions\n", targ->thread_id, targ->num_ops);
    return NULL;
}

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1000000000.0;
}

int main() {
    // Init the list
    list_t list;
    list_init(&list);

    // Create thread args
    pthread_t threads[NUM_THREADS];
    thread_arg_t args[NUM_THREADS];

    printf("Starting concurrent insertions...\n");
    double start_time = get_time();

    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].list = &list;
        args[i].thread_id = i;
        args[i].start_val = i * OPERATIONS_PER_THREAD; // Non-overlapping ranges
        args[i].num_ops = OPERATIONS_PER_THREAD;
        pthread_create(&threads[i], NULL, thread_ops, &args[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    double end_time = get_time();

    
    printf("Time taken: %.4f seconds\n", end_time - start_time);
    
    int total_count = list_count(&list);
    printf("Total elements in list: %d\n", total_count);
    
    if (total_count == NUM_THREADS * OPERATIONS_PER_THREAD) {
        printf("All insertions successful!\n");
    } else {
        printf("Error: Expected %d elements, got %d\n", 
               NUM_THREADS * OPERATIONS_PER_THREAD, total_count);
    }
    
    // Debug: show first few elements
    // list_print(&list, 10);
    
    // Test some lookups
    int test_values[] = {0, 500, 999, 1500, 9999};
    for (int i = 0; i < 5; i++) {
        int found = list_lookup(&list, test_values[i]);
        printf("Looking for %d: %s\n", 
               test_values[i], found ? "FOUND" : "NOT FOUND");
    }
    
    // Clean up
    list_destroy(&list);

    return 0;
}