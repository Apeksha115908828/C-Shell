// file: threaded_buggy.c
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_THREADS 4
int counter = 0;  // Shared without synchronization
pthread_mutex_t lock;
void* thread_func(void* arg) {
    // pthread_mutex_lock(&lock);  // Lock
    for (int i = 0; i < 100000; ++i) {
        counter++; // Race condition here
    }
    // pthread_mutex_unlock(&lock);  // Unlock
    int* leaky = malloc(sizeof(int));  // Memory leak
    *leaky = 42;

    pthread_exit(NULL);
}

int main() {
    pthread_t threads[NUM_THREADS];
    pthread_mutex_init(&lock, NULL);
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_create(&threads[i], NULL, thread_func, NULL);
    }

    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], NULL);
    }

    printf("Final counter: %d\n", counter);
    return 0;
}
