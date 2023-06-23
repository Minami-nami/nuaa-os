
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#define NUM_THREADS 10
pthread_t threads[NUM_THREADS];
long long arg[NUM_THREADS];
long long ret[NUM_THREADS];
void     *thread_func(void *arg) {
    int       start = *(int *)arg;
    long long sum   = 0;
    for (int i = start; i < start + 10; ++i) {
        sum += i;
    }
    long long *ret = malloc(sizeof(long long));
    *ret           = sum;
    return ret;
}
int main() {
    for (int i = 0; i < NUM_THREADS; ++i) {
        arg[i] = i * 10 + 1;
        pthread_create(&threads[i], NULL, thread_func, (void *)&arg[i]);
    }
    int sum = 0;
    for (int i = 0; i < NUM_THREADS; ++i) {
        long long *m;
        pthread_join(threads[i], (void *)&m);
        sum += *m;
    }
}