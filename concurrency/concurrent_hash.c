/**
 * OSTEP - Concurrency
 * 
 * Concurrent hash table with per-bucket locking
 * This is somewhat Redis/Memcached's basic (they have better hash func and dynamic scaling)
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define NUM_BUCKETS 101
#define NUM_THREADS 4
#define OPS_PER_THREAD 1000

// Node for linked list in each bucket
typedef struct node
{
    int key;
    int value;
    struct node *next;
} node_t;

// A single bucket (linked list with its own lock)
typedef struct 
{
    node_t *head;
    pthread_mutex_t lock;
} bucket_t;

// Hash table struc
typedef struct  
{
    bucket_t buckets[NUM_BUCKETS];
    int num_buckets;
} hashtable_t;

// Init hash table
void hash_init(hashtable_t *ht) {
    ht->num_buckets = NUM_BUCKETS;

    for (int i = 0; i < NUM_BUCKETS; i++) {
        ht->buckets[i].head = NULL;
        pthread_mutex_init(&ht->buckets[i].lock, NULL);
    }

    printf("Hash table init with %d buckets\n", NUM_BUCKETS);
}

// Simple hash func
int hash_func(hashtable_t *ht, int key) {
    return abs(key) % ht->num_buckets;
}

// Insert key-value pair
int hash_insert(hashtable_t *ht, int key, int value) {
    int bucket_idx = hash_func(ht, key);
    bucket_t *bucket = &ht->buckets[bucket_idx];

    // Create new node (outside the critical section)
    node_t *new_node = (node_t *)malloc(sizeof(node_t));
    if (new_node == NULL)
        return -1;

    new_node->key = key;
    new_node->value = value;

    pthread_mutex_lock(&bucket->lock);

    // Check if key already exists
    node_t *cur = bucket->head;
    while (cur != NULL) {
        if (cur->key == key) {
            cur->value = value;
            pthread_mutex_unlock(&bucket->lock);
            free(new_node);
            return 1;
        }
        cur = cur->next;
    }

    // Add new node at head
    new_node->next = bucket->head;
    bucket->head = new_node;

    pthread_mutex_unlock(&bucket->lock);

    return 0; // Inserted new
}

// Lookup a key
int hash_lookup(hashtable_t *ht, int key, int *value) {
    int bucket_idx = hash_func(ht, key);
    bucket_t *bucket = &ht->buckets[bucket_idx];

    pthread_mutex_lock(&bucket->lock);

    node_t *cur = bucket->head;
    while (cur != NULL) {
        if (cur->key == key) {
            *value = cur->value;
            pthread_mutex_unlock(&bucket->lock);
            return 1;
        }
        cur = cur->next;
    }
    pthread_mutex_unlock(&bucket->lock);
    return 0;
}

// Delete a key
int hash_delete(hashtable_t *ht, int key) {
    int bucket_idx = hash_func(ht, key);
    bucket_t *bucket = &ht->buckets[bucket_idx];

    pthread_mutex_lock(&bucket->lock);

    node_t *cur = bucket->head;
    node_t *prev = NULL;

    while (cur != NULL) {
        if (cur->key == key) {
            if (prev == NULL) {
                bucket->head = cur->next;
            } else {
                prev->next = cur->next;
            }
            pthread_mutex_unlock(&bucket->lock);
            free(cur);
            return 1;
        }
        prev = cur;
        cur = cur->next;
    }

    pthread_mutex_unlock(&bucket->lock);
    return 0;
}

// Cleanup hash table
void hash_destroy(hashtable_t *ht) {
    for (int i = 0; i < NUM_BUCKETS; i++) {
        pthread_mutex_lock(&ht->buckets[i].lock);

        node_t *cur = ht->buckets[i].head;
        while (cur != NULL) {
            node_t *tmp = cur;
            cur = cur->next;
            free(tmp);
        }
        ht->buckets[i].head = NULL;

        pthread_mutex_unlock(&ht->buckets[i].lock);
        pthread_mutex_destroy(&ht->buckets[i].lock);
    }
}

// Print bucket details
void hash_stats(hashtable_t *ht) {
    int total = 0, max_chain = 0, non_empty_bucket = 0;

    for (int i = 0; i < NUM_BUCKETS; i++) {
        pthread_mutex_lock(&ht->buckets[i].lock);

        int chain_len = 0;
        node_t *cur = ht->buckets[i].head;
        while (cur != NULL) {
            chain_len++;
            cur = cur->next;
        }

        if (chain_len > 0) {
            non_empty_bucket++;
            printf("Bucket %d: %d items\n", i, chain_len);
        }

        total += chain_len;
        if (chain_len > max_chain) {
            max_chain = chain_len;
        }

        pthread_mutex_unlock(&ht->buckets[i].lock);
    }

    printf("Total items: %d\n", total);
    printf("Non-empty buckets: %d/%d\n", non_empty_bucket, NUM_BUCKETS);
    printf("Average chain length: %.2f\n", 
           non_empty_bucket > 0 ? (float)total / non_empty_bucket : 0);
    printf("Max chain length: %d\n", max_chain);
}

// Thread args struct
typedef struct 
{
    hashtable_t *ht;
    int thread_id;
    int num_ops;
    int *ops_cnt;
} thread_arg_t;

void* thread_worker(void* arg) {
    thread_arg_t *targ = (thread_arg_t *)arg;
    int success = 0;

    // Each thread works on a range of keys to reduce conflict
    int key_base = targ->thread_id * 1000;

    for (int i = 0; i < targ->num_ops; i++) {
        int key = key_base + (rand() % 500);
        int value = rand() % 1000;
        int op = rand() % 100;

        if (op < 60) {
            hash_insert(targ->ht, key, value);
            success++;
        } else if (op < 90) {
            int found_value;
            if (hash_lookup(targ->ht, key, &found_value)) {
                success++;
            }
        } else {
            if (hash_delete(targ->ht, key)) {
                success++;
            }
        }
    }

    *targ->ops_cnt = success;
    printf("Thread %d: Completed %d operations (%d successful)\n",
           targ->thread_id, targ->num_ops, success);

    return NULL;
}

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1000000000.0;
}

void demonstrate_concurrency(hashtable_t *ht) {
    // Insert some data
    for (int i = 0; i < 5; i++) {
        hash_insert(ht, i, i * 10);
        hash_insert(ht, i + 20, i * 10);  
    }
    
    printf("Init setup: Added keys 0-4 and 20-24\n");
    printf("Key 0 goes to bucket %d\n", hash_func(ht, 0));
    printf("Key 20 goes to bucket %d\n", hash_func(ht, 20));
}

int main() {
    srand(time(NULL));
    
    // Init hash table
    hashtable_t ht;
    hash_init(&ht);
    
    // How different buckets can be accessed concurrently
    demonstrate_concurrency(&ht);
    
    pthread_t threads[NUM_THREADS];
    thread_arg_t args[NUM_THREADS];
    int operation_counts[NUM_THREADS];
    
    double start_time = get_time();
    
    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].ht = &ht;
        args[i].thread_id = i;
        args[i].num_ops = OPS_PER_THREAD;
        args[i].ops_cnt = &operation_counts[i];
        pthread_create(&threads[i], NULL, thread_worker, &args[i]);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    double end_time = get_time();
    
    printf("Time: %.4f seconds\n", end_time - start_time);
    printf("Total Ops: %d\n", NUM_THREADS * OPS_PER_THREAD);
    printf("ops/second: %.0f\n", 
           (NUM_THREADS * OPS_PER_THREAD) / (end_time - start_time));
    
    int total_successful = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        total_successful += operation_counts[i];
    }
    printf("Successful operations: %d\n", total_successful);
    
    hash_stats(&ht);
    
    int test_keys[] = {0, 1000, 2000, 3000};
    for (int i = 0; i < 4; i++) {
        int value;
        if (hash_lookup(&ht, test_keys[i], &value)) {
            printf("Key %d found with value %d (bucket %d)\n", 
                   test_keys[i], value, hash_func(&ht, test_keys[i]));
        } else {
            printf("Key %d not found\n", test_keys[i]);
        }
    }
    
    // Clean up
    hash_destroy(&ht);

    return 0;
}
