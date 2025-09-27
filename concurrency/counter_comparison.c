/**
 * OSTEP - Concurrency
 * 
 * The difference between unsafe/safe counters
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define NUM_THREADS 4
#define INCREMENTS_PER_THREAD 1000000

// Unsafe
typedef struct
{
    int value;
} unsafe_counter_t;

// Thread-safe counter (with mutex)
typedef struct 
{
    int value;
    pthread_mutex_t lock;
}safe_counter_t;

// Global counter for testing
unsafe_counter_t unsafe_counter = {0};
safe_counter_t safe_counter;

void safe_counter_init(safe_counter_t *c) {
    c->value = 0;
    pthread_mutex_init(&c->lock, NULL);
}

// Thread func for unsafe_counter
void* unsafe_increment(void* arg) {
    for (int i = 0; i < INCREMENTS_PER_THREAD; i++) {
        unsafe_counter.value++; // ! Race condition
    }
    return NULL;
}

// Thread func for safe_counter
void* safe_increment(void* arg) {
    for (int i = 0; i < INCREMENTS_PER_THREAD; i++) {
        pthread_mutex_lock(&safe_counter.lock); // Acquire lock
        safe_counter.value++;  // <-- This is critical section
        pthread_mutex_unlock(&safe_counter.lock); // Free the lock
    }
    return NULL;
}

// Get the time
double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1000000000.0;
}

int main() {
    pthread_t threads[NUM_THREADS];
    double start_time, end_time;

    // Test unsafe counter (will likely get the wrong result)
    printf("Test Unsafe counter\n");
    unsafe_counter.value = 0;

    start_time = get_time();
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, unsafe_increment, NULL);
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    end_time = get_time();

    printf("Value: %d\n", unsafe_counter.value);
    printf("Time: %.3f seconds\n", end_time - start_time);
    printf("Error: %d lost\n\n",
           NUM_THREADS * INCREMENTS_PER_THREAD - unsafe_counter.value);

    printf("-------------------------------------\n");

    // Test thread-safe counter 
    printf("Test safe counter\n");
    safe_counter_init(&safe_counter);

    start_time = get_time();
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, safe_increment, NULL);
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    end_time = get_time();

    printf("Value: %d\n", safe_counter.value);
    printf("Time: %.3f seconds\n", end_time - start_time);
    
    // Clean
    pthread_mutex_destroy(&safe_counter.lock);

    return 0;
}
