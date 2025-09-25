/**
 * OSTEP - Concurrenct 
 * Essential pthread API and common pitfalls
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// Thread creation and joining
typedef struct  
{
    int a;
    int b;
} thread_arg_t;

typedef struct 
{
    int result;
} thread_ret_t;

void* worker_thread(void* arg) {
    thread_arg_t* args = (thread_arg_t *)arg;

    // Do the work with args->a and args->b
    printf("Thread working with: %d, &d\n", args->a, args->b);

    // KEY: Return heap allocated memory, not stack
    thread_ret_t *ret = malloc(sizeof(thread_ret_t));
    ret->result = args->a + args->b;
    return (void *)ret;
}

void thread_creation() {
    pthread_t tid;
    thread_arg_t args = {10, 20};
    thread_ret_t *ret;

    // Create
    int rc = pthread_create(&tid, NULL, worker_thread, &args);
    assert(rc == 0);

    // Wait for completion and get result
    rc = pthread_join(tid, (void **) &ret);
    assert(rc == 0);

    printf("Thread returned: %d\n", ret->result);
    free(ret); // Never forget to free
}

// Common mistakes to avoid

// Wrong: Return stack address
void* bad_thread_1(void* arg) {
    int local_var = 42;
    return (void *)&local_var; // Stack var dies when thread exits
}

// Wrong: Passing loop var address
void bad_thread_2() {
    pthread_t threads[10];
    for (int i = 0; i < 10; i++) {
        // Bug: All threads get same addr, but value keep changing
        pthread_create(&threads[i], NULL, worker_thread, &i);
    }
}

// Correct: Passing value directly(for simple types) 
void* good_thread(void* arg) {
    int val = (int)arg; // cast directly
    printf("Got val: %d\n", val);
    return NULL;
}

void good_thread_creation() {
    pthread_t tid;
    pthread_create(&tid, NULL, good_thread, (void *)42);
    pthread_join(tid, NULL);
}
