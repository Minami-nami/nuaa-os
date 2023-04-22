#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define NUM_THREADS 2
#define ARRAY_SIZE 1000

struct sort_arg {
    int *array;
    int *start;
    int *end;
};

void *thread_function(void *_arg) {
    // sort the array of [start, end)
    struct sort_arg *arg   = (struct sort_arg *)_arg;
    int             *array = arg->array;
    int             *start = arg->start;
    int             *end   = arg->end;
    // selection sort
    for (int *i = start; i < end; i++) {
        int *min = i;
        for (int *j = i + 1; j <= end; j++) {
            if (*j < *min) {
                min = j;
            }
        }
        int temp = *i;
        *i       = *min;
        *min     = temp;
    }
    return NULL;
}

void Gen_Random(int *a, int n, int l, int r) {
    srand(time(0));
    for (int i = 0; i < n; i++) {
        a[i] = rand() % (r - l + 1) + l;
    }
}

void Merge(int *a, int l, int mid, int r) {
    // merge [l, mid] and [mid + 1, r]
    int *temp = malloc(sizeof(int) * (r - l + 1));
    int  i = l, j = mid + 1, k = 0;
    while (i <= mid && j <= r) {
        if (a[i] <= a[j]) {
            temp[k++] = a[i++];
        }
        else {
            temp[k++] = a[j++];
        }
    }
    while (i <= mid) {
        temp[k++] = a[i++];
    }
    while (j <= r) {
        temp[k++] = a[j++];
    }
    memcpy(a + l, temp + l, (r - l + 1) * sizeof(int));
    free(temp);
}

void Print_Array(int *array, int n) {
    FILE *result_file = fopen("result.txt", "w");
    for (int i = 0; i < n; i++) {
        fprintf(result_file, "%d ", array[i]);
    }
    fprintf(result_file, "\n");
}

int main(int argc, char *argv[]) {
    pthread_t       thread[NUM_THREADS];
    struct sort_arg arg[NUM_THREADS];
    struct timeval  starttime, endtime;
    int            *array  = malloc(sizeof(int) * ARRAY_SIZE);
    int             differ = ARRAY_SIZE / NUM_THREADS;
    Gen_Random(array, ARRAY_SIZE, 0, INT_LEAST32_MAX - 1);
    gettimeofday(&starttime, 0);
    for (int i = 0; i < NUM_THREADS; ++i) {
        arg[i].array = array;
        arg[i].start = array + i * differ;
        arg[i].end   = array + (i + 1) * differ;
        pthread_create(&thread[i], NULL, thread_function, &arg[i]);
    }
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(thread[i], NULL);
    }
    Merge(array, 0, differ, ARRAY_SIZE);
    gettimeofday(&endtime, 0);
    double total_time = (endtime.tv_sec - starttime.tv_sec) * 1000.0 + (endtime.tv_usec - starttime.tv_usec) / 1000.0;
    Print_Array(array, ARRAY_SIZE);
    printf("total time: %lf ms, thread %d", total_time, NUM_THREADS);
    return 0;
}