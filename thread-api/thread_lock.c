/**
 * OSTEP - Concurrency
 * Locks and condition var
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

int shared_counter = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// Thread-safe example(increment)
void* safe_increment(void* arg) {
    for (int i = 0; i < 1000000; i++) {
        pthread_mutex_lock(&lock);
        shared_counter++; // this is critical section
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

// condition var example
typedef struct  
{
    int ready;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} state_t;

state_t state = {
    .ready = 0,
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER
};

void* waiter(void* arg) {
    pthread_mutex_lock(&state.lock);

    // Use while, not if!
    while (state.ready == 0) {
        pthread_cond_wait(&state.cond, &state.lock);
        // wait() releases lock and sleeps
        // Re-acquires lock when woken up
    }

    printf("Condition met\n");
    pthread_mutex_unlock(&state.lock);
    return NULL;
}

void* signaler(void* arg) {
    sleep(1);

    pthread_mutex_lock(&state.lock);
    state.ready = 1;
    pthread_cond_signal(&state.cond); // Wait waiter
    pthread_mutex_unlock(&state.lock);

    return NULL;
}

// Pattern-consumer pattern
#define BUFFER_SIZE 10

typedef struct 
{
    int buffer[BUFFER_SIZE];
    int count;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} buffer_t;

buffer_t buf = {
    .count = 0,
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .not_empty = PTHREAD_COND_INITIALIZER,
    .not_full = PTHREAD_COND_INITIALIZER
};

void* producer(void* arg) {
    for (int i = 0; i < 100; i++) {
        pthread_mutex_lock(&buf.lock);

        while (buf.count == BUFFER_SIZE) { // buffer full
            pthread_cond_wait(&buf.not_full, &buf.lock);
        }

        buf.buffer[buf.count++] = i;
        pthread_cond_signal(&buf.not_empty);
        pthread_mutex_unlock(&buf.lock);
    }
    return NULL;
}

void* consumer(void* arg) {
    for (int i = 0; i < 100; i++) {
        pthread_mutex_lock(&buf.lock);

        while(buf.count == 0) { // buffer empty-> wait
            pthread_cond_wait(&buf.not_empty, &buf.lock);
        }

        int item = buf.buffer[--buf.count];
        printf("Consumed: %d\n", item);
        pthread_cond_signal(&buf.not_full);
        pthread_mutex_unlock(&buf.lock);
    }
    return NULL;
}

int main() {
    pthread_t t1, t2;

    pthread_create(&t1, NULL, safe_increment, NULL);
    pthread_create(&t2, NULL, safe_increment, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    printf("Counter with lock: %d\n", shared_counter);

    // Test condition var
    pthread_create(&t1, NULL, waiter, NULL);
    pthread_create(&t2, NULL, signaler, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    return 0;
}
