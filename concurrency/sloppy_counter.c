/**
 * OSTEP - Concurrency
 * 
 * Sloppy counter for better scalability
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#define NUM_CPUS 4
#define NUM_THREADS 4
// #define INCREMENTS_PER_THREAD 1000007
#define INCREMENTS_PER_THREAD 100000

// Sloppy counter structure
typedef struct  
{
    int global;  // Global count
    pthread_mutex_t glock;   // Global lock
    int local[NUM_CPUS];   // Per-CPU local counts
    pthread_mutex_t llock[NUM_CPUS];    // Per-CPU locks
    int threshold;    // Update threshold (S value)
} sloppy_counter_t;

// Thread arg structure
typedef struct  
{
    sloppy_counter_t *counter;
    int thread_id;
    int num_increments;
} thread_arg_t;

// Init
void sloppy_init(sloppy_counter_t *c, int threshold) {
    c->threshold = threshold;
    c->global = 0;
    pthread_mutex_init(&c->glock, NULL);

    for (int i = 0; i < NUM_CPUS; i++) {
        c->local[i] = 0;
        pthread_mutex_init(&c->llock[i], NULL);
    }
}

// Update
void sloppy_update(sloppy_counter_t *c, int cpu_id, int amt) {
    // Lock the local counter for this CPU
    pthread_mutex_lock(&c->llock[cpu_id]);
    c->local[cpu_id] += amt;

    // If local counter exceed the threshold, transfer to global
    if (c->local[cpu_id] >= c->threshold) {
        pthread_mutex_lock(&c->glock);
        c->global += c->local[cpu_id];
        pthread_mutex_unlock(&c->glock);

        // Remember to reset the local counter
        c->local[cpu_id] = 0;
        printf("[CPU %d] transferred to global (threshold %d reached)\n", cpu_id, c->threshold);
    }

    pthread_mutex_unlock(&c->llock[cpu_id]);
}

// Get rough value (only read global counter)
int sloppy_get_approx(sloppy_counter_t *c) {
    pthread_mutex_lock(&c->glock);
    int val = c->global;
    pthread_mutex_unlock(&c->glock);
    return val;
}

// Get precise value (lock everything)
int sloppy_get_precise(sloppy_counter_t *c) {
    pthread_mutex_lock(&c->glock);
    int total = c->global;

    for (int i = 0; i < NUM_CPUS; i++) {
        pthread_mutex_lock(&c->llock[i]);
        total += c->local[i];
        pthread_mutex_unlock(&c->llock[i]);
    }

    pthread_mutex_unlock(&c->glock);
    return total;
}

// Sloppy counter
void* sloppy_increment(void* arg) {
    thread_arg_t *targ = (thread_arg_t *)arg;
    int cpu_id = targ->thread_id % NUM_CPUS;

    for (int i = 0; i < targ->num_increments; i++) {
        sloppy_update(targ->counter, cpu_id, 1);
    }

    return NULL;
}

// Clean up
void sloppy_destroy(sloppy_counter_t *c) {
    pthread_mutex_destroy(&c->glock);
    for (int i = 0; i < NUM_CPUS; i++) {
        pthread_mutex_destroy(&c->llock[i]);
    }
}

// Get the time
double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1000000000.0;
}

int main() {
    int thresholds[] = {1, 10, 100, 1000, 10000};
    int num_tests = sizeof(thresholds) / sizeof(thresholds[0]);

    for (int t = 0; t < num_tests; t++) {
        int S = thresholds[t];
        printf("Testing with threshold S = %d:\n", S);

        sloppy_counter_t counter;
        sloppy_init(&counter, S);

        pthread_t threads[NUM_THREADS];
        thread_arg_t args[NUM_THREADS];

        double start_time = get_time();

        // Create threads
        for (int i = 0; i < NUM_THREADS; i++) {
            args[i].counter = &counter;
            args[i].thread_id = i;
            args[i].num_increments = INCREMENTS_PER_THREAD;
            pthread_create(&threads[i], NULL, sloppy_increment, &args[i]);
        }

        for (int i = 0; i < NUM_THREADS; i++) {
            pthread_join(threads[i], NULL);
        }

        double end_time = get_time();

        // Get results
        int approx_val = sloppy_get_approx(&counter);
        int precise_val = sloppy_get_precise(&counter);

        printf("Time: %.4f seconds\n", end_time - start_time);
        printf("Approx val (global only): %d\n", approx_val);
        printf("Precise val (all): %d\n", precise_val);
        printf("Local values: [");
        for (int i = 0; i < NUM_CPUS; i++) {
            printf("%d", counter.local[i]);
            if (i < NUM_CPUS - 1) printf(", ");
        }
        printf("]\n\n");

        sloppy_destroy(&counter);
    }

    return 0;
}
