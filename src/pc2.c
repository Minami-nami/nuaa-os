#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define ITEM_COUNT 8
#define INITIAL 'a'
#define CAPACITY 4

char buffer[2][CAPACITY];

pthread_mutex_t mutex_buffer1, mutex_buffer2;

sem_t sem_buffer1_empty, sem_buffer1_full, sem_buffer2_empty, sem_buffer2_full;

int start[2] = { 0, 0 }, end[2] = { 0, 0 };

char get_item(int ch) {
    char ret  = buffer[ch][start[ch]];
    start[ch] = (start[ch] + 1) % CAPACITY;
    return ret;
}

void put_item(int ch, char item) {
    buffer[ch][end[ch]] = item;
    end[ch]             = (end[ch] + 1) % CAPACITY;
}

void *customer() {
    char item;
    for (int i = 0; i < ITEM_COUNT; i++) {
        sem_wait(&sem_buffer2_full);
        pthread_mutex_lock(&mutex_buffer2);
        item = get_item(1);
        printf("%c\n", item);
        pthread_mutex_unlock(&mutex_buffer2);
        sem_post(&sem_buffer2_empty);
    }
    return NULL;
}

void *producer() {
    char item = INITIAL;
    for (int i = 0; i < ITEM_COUNT; i++, item++) {
        sem_wait(&sem_buffer1_empty);
        pthread_mutex_lock(&mutex_buffer1);
        put_item(0, item);
        pthread_mutex_unlock(&mutex_buffer1);
        sem_post(&sem_buffer1_full);
    }
    return NULL;
}

void *calculator() {
    char item;
    for (int i = 0; i < ITEM_COUNT; i++) {
        sem_wait(&sem_buffer1_full);
        pthread_mutex_lock(&mutex_buffer1);
        item = get_item(0);
        pthread_mutex_unlock(&mutex_buffer1);
        sem_post(&sem_buffer1_empty);

        item = item - 'a' + 'A';

        sem_wait(&sem_buffer2_empty);
        pthread_mutex_lock(&mutex_buffer2);
        put_item(1, item);
        pthread_mutex_unlock(&mutex_buffer2);
        sem_post(&sem_buffer2_full);
    }
    return NULL;
}

int main() {
    pthread_t producer_thread, calculator_thread, customer_thread;

    pthread_mutex_init(&mutex_buffer1, NULL);
    pthread_mutex_init(&mutex_buffer2, NULL);

    sem_init(&sem_buffer1_empty, 0, CAPACITY);
    sem_init(&sem_buffer1_full, 0, 0);
    sem_init(&sem_buffer2_empty, 0, CAPACITY);
    sem_init(&sem_buffer2_full, 0, 0);

    pthread_create(&producer_thread, NULL, producer, NULL);
    pthread_create(&calculator_thread, NULL, calculator, NULL);
    pthread_create(&customer_thread, NULL, customer, NULL);

    pthread_join(producer_thread, NULL);
    pthread_join(calculator_thread, NULL);
    pthread_join(customer_thread, NULL);

    return 0;
}