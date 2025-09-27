/**
 * OSTEP - Concurrency
 * 
 * Michael & Scott style concurrent queue with separate head/tail lock
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#define NUM_PRODUCERS 2
#define NUM_CONSUMERS 2
#define ITEMS_PER_PRODUCER 1000

// Node structure for queue
typedef struct node
{
    int value;
    struct node *next;
} node_t;

// Concurrent queue with two locks
typedef struct 
{
    node_t *head;  // For dequeue
    node_t *tail;  // For enqueue
    pthread_mutex_t head_lock;
    pthread_mutex_t tail_lock;
} queue_t;

// Init queue with dummy node
void queue_init(queue_t *q) {
    // Create dummy node
    node_t *dummy = (node_t *)malloc(sizeof(node_t));
    dummy->next = NULL;

    q->head = dummy;
    q->tail = dummy;

    pthread_mutex_init(&q->head_lock, NULL);
    pthread_mutex_init(&q->tail_lock, NULL);

    printf("Queue init with dummy node at %p\n", (void *)dummy);
}

// Enqueue (add to tail)
void q_enqueue(queue_t *q, int value) {
    // Create new node (outside critical section)
    node_t *new_node = (node_t *)malloc(sizeof(node_t));
    new_node->value = value;
    new_node->next = NULL;
    
    // Only lock tail
    pthread_mutex_lock(&q->tail_lock);

    // Add to tail
    q->tail->next = new_node;
    q->tail = new_node;

    pthread_mutex_unlock(&q->tail_lock);
}

// Dequeue (remove from head)
int q_dequeue(queue_t *q, int *value) {
    // Only lock head for dequeue
    pthread_mutex_lock(&q->head_lock);

    node_t *dummy = q->head;
    node_t *new_head = dummy->next;

    if (new_head == NULL) {
        pthread_mutex_unlock(&q->head_lock);
        return -1;
    }

    *value = new_head->value;
    q->head = new_head;

    pthread_mutex_unlock(&q->head_lock);

    free(dummy);

    return 0;
}

// Check if queue is empty
int q_is_empty(queue_t *q) {
    pthread_mutex_lock(&q->head_lock);
    int empty = (q->head->next == NULL);
    pthread_mutex_unlock(&q->head_lock);
    return empty;
}

// Get queue size (we need to lock both)
int q_size(queue_t *q) {
    pthread_mutex_lock(&q->head_lock);
    pthread_mutex_lock(&q->tail_lock);

    int cnt = 0;
    node_t *cur = q->head->next;
    while (cur != NULL) {
        cnt++;
        cur = cur->next;
    }

    pthread_mutex_unlock(&q->head_lock);
    pthread_mutex_unlock(&q->tail_lock);

    return cnt;
}

// Cleanup queue
void q_destroy(queue_t *q) {
    int val;
    while (q_dequeue(q, &val) == 0) {
    }

    free(q->head);

    pthread_mutex_destroy(&q->head_lock);
    pthread_mutex_destroy(&q->tail_lock);
}

// Producer/Consumer thread args
typedef struct 
{
    queue_t *queue;
    int prod_id;
    int num_items;
} prod_arg_t;

typedef struct 
{
    queue_t *queue;
    int con_id;
    int *con_cnt;
} con_arg_t;

void* prod_thread(void* arg) {
    prod_arg_t *parg = (prod_arg_t *)arg;

    printf("Producer %d: Starting to produce %d items\n",
           parg->prod_id, parg->num_items);

    for (int i = 0; i < parg->num_items; i++) {
        // Generate unique val for each producer
        int val = parg->prod_id * 10000 + i;
        q_enqueue(parg->queue, val);

        // Print progress
        if ((i + 1) % 250 == 0) {
            printf("Producer %d: Produced %d items\n",
                   parg->prod_id, i + 1);
        }

        // Simulate small delay
        if (i % 100 == 0) {
            usleep(100); // 0.1ms
        }
    }

    printf("Producer %d: Finished producing %d items\n",
           parg->prod_id, parg->num_items);
    return NULL;
}

void* con_thread(void* arg) {
    con_arg_t *carg = (con_arg_t *)arg;
    int local_cnt = 0;
    int val;

    printf("Consumer %d: Starting consumption\n", carg->con_id);

    while (1) {
        if (q_dequeue(carg->queue, &val) == 0) {
            local_cnt++;

            if (local_cnt % 250 == 0) {
                printf("Consumer %d: Consumed %d items (latest: %d)\n",
                       carg->con_id, local_cnt, val);
            }
        } else {
            // Queue is empty - check if we should exit
            // In real case, we'd use condition var here
            usleep(1000);

            // If queue stays empty for a bit, exit
            static int empty_cnt = 0;
            if (q_is_empty(carg->queue)) {
                empty_cnt++;
                if (empty_cnt > 100) {
                    break;
                }
            } else {
                empty_cnt = 0;
            }
        }
    }

    *carg->con_cnt = local_cnt;
    printf("Consumer %d: Finished, consumed %d items\n",
           carg->con_id, local_cnt);
    return NULL;
}

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1000000000.0;
}

int main() {
    printf("Producers: %d, Consumers: %d\n", NUM_PRODUCERS, NUM_CONSUMERS);
    printf("Items per producer: %d\n", ITEMS_PER_PRODUCER);
    printf("Total items: %d\n\n", NUM_PRODUCERS * ITEMS_PER_PRODUCER);
    
    // Init queue
    queue_t queue;
    queue_init(&queue);
    
    // Create producer and consumer threads
    pthread_t producers[NUM_PRODUCERS];
    pthread_t consumers[NUM_CONSUMERS];
    prod_arg_t prod_args[NUM_PRODUCERS];
    con_arg_t cons_args[NUM_CONSUMERS];
    int consumed_counts[NUM_CONSUMERS];
    
    double start_time = get_time();
    
    // Start consumers first
    printf("Starting consumers...\n");
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        cons_args[i].queue = &queue;
        cons_args[i].con_id = i;
        cons_args[i].con_cnt = &consumed_counts[i];
        pthread_create(&consumers[i], NULL, con_thread, &cons_args[i]);
    }
    
    // Start producers
    printf("Starting producers...\n");
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        prod_args[i].queue = &queue;
        prod_args[i].prod_id = i;
        prod_args[i].num_items = ITEMS_PER_PRODUCER;
        pthread_create(&producers[i], NULL, prod_thread, &prod_args[i]);
    }
    
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        pthread_join(producers[i], NULL);
    }
    printf("\nAll producers finished\n");
    
    // Give consumers time to finish consuming
    sleep(1);
    
    // Cancel consumers
    // In real case, use proper signaling
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        pthread_cancel(consumers[i]);
    }
    
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        pthread_join(consumers[i], NULL);
    }
    
    double end_time = get_time();
    
    printf("Time: %.3f seconds\n", end_time - start_time);
    
    int total_consumed = 0;
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        printf("Consumer %d consumed: %d items\n", i, consumed_counts[i]);
        total_consumed += consumed_counts[i];
    }
    
    printf("\nTotal produced: %d\n", NUM_PRODUCERS * ITEMS_PER_PRODUCER);
    printf("Total consumed: %d\n", total_consumed);
    
    if (total_consumed == NUM_PRODUCERS * ITEMS_PER_PRODUCER) {
        printf("Success: All items were consumed!\n");
    } else {
        printf("Warning: Not all items consumed\n");
        printf("Items remaining in queue: %d\n", q_size(&queue));
    }
    
    // Clean up
    q_destroy(&queue);

    return 0;
}
