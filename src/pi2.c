#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#define MAXITER 1000000000
#define NUM_THREADS 1000

struct LeibNiz_Arg {
    int start;
    int end;
};

struct LeibNiz_Result {
    double result;
};

void *thread_function(void *_arg) {
    struct LeibNiz_Result *result = malloc(sizeof(struct LeibNiz_Result));
    struct LeibNiz_Arg    *arg    = (struct LeibNiz_Arg *)_arg;
    result->result                = 0.0;
    for (int i = arg->start; i <= arg->end; ++i) {
        result->result += ((i & 1) == 0 ? -1.0 : 1.0) / ((i << 1) - 1.0);
    }
    return result;
}

int main(int argc, char *argv[]) {
    pthread_t          thread[NUM_THREADS];
    struct LeibNiz_Arg arg[NUM_THREADS];
    double             leibniz = 0.0;
    int                differ  = MAXITER / NUM_THREADS;
    struct timeval     starttime, endtime;
    gettimeofday(&starttime, 0);
    for (int i = 0; i < NUM_THREADS; ++i) {
        arg[i].start = i * differ + 1;
        arg[i].end   = (i + 1) * differ;
        pthread_create(&thread[i], NULL, thread_function, &arg[i]);
    }

    for (int i = 0; i < NUM_THREADS; ++i) {
        struct LeibNiz_Result *result;
        pthread_join(thread[i], (void **)&result);
        leibniz += result->result;
        free(result);
    }
    gettimeofday(&endtime, 0);
    double total_time = (endtime.tv_sec - starttime.tv_sec) * 1000.0 + (endtime.tv_usec - starttime.tv_usec) / 1000.0;
    printf("pi/4 = %lf, after %d iterations\n", leibniz, MAXITER);
    printf("total time: %lf ms, thread %d\n", total_time, NUM_THREADS);
    return 0;
}