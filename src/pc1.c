#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

/*
+ 系统中有3个线程：生产者、计算者、消费者
+ 系统中有2个容量为4的缓冲区：buffer1、buffer2
+ 生产者生产'a'、'b'、'c'、‘d'、'e'、'f'、'g'、'h'八个字符，放入到buffer1
+ 计算者从buffer1取出字符，将小写字符转换为大写字符，放入到buffer2
+ 消费者从buffer2取出字符，将其打印到屏幕上
*/

#define ITEM_COUNT 8
#define INITIAL 'a'
#define CAPACITY 4

char buffer[2][CAPACITY];

pthread_mutex_t mutex_buffer1, mutex_buffer2;
pthread_cond_t  cond_buffer1_empty, cond_buffer1_full, cond_buffer2_empty, cond_buffer2_full;

int start[2] = { 0, 0 }, end[2] = { 0, 0 };

int buffer_is_full(int ch) {
    return (end[ch] + 1) % CAPACITY == start[ch];
}

int buffer_is_empty(int ch) {
    return start[ch] == end[ch];
}

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
        pthread_mutex_lock(&mutex_buffer2);
        while (buffer_is_empty(1)) pthread_cond_wait(&cond_buffer2_full, &mutex_buffer2);
        item = get_item(1);
        printf("%c\n", item);
        pthread_cond_signal(&cond_buffer2_empty);
        pthread_mutex_unlock(&mutex_buffer2);
    }
    return NULL;
}

void *producer() {
    char item = INITIAL;
    for (int i = 0; i < ITEM_COUNT; i++, item++) {
        pthread_mutex_lock(&mutex_buffer1);
        while (buffer_is_full(0)) pthread_cond_wait(&cond_buffer1_empty, &mutex_buffer1);
        put_item(0, item);
        pthread_cond_signal(&cond_buffer1_full);
        pthread_mutex_unlock(&mutex_buffer1);
    }
    return NULL;
}

void *calculator() {
    char item;
    for (int i = 0; i < ITEM_COUNT; i++) {
        pthread_mutex_lock(&mutex_buffer1);
        while (buffer_is_empty(0)) pthread_cond_wait(&cond_buffer1_full, &mutex_buffer1);
        item = get_item(0);
        pthread_cond_signal(&cond_buffer1_empty);
        pthread_mutex_unlock(&mutex_buffer1);

        item = item - 'a' + 'A';

        pthread_mutex_lock(&mutex_buffer2);
        while (buffer_is_full(1)) pthread_cond_wait(&cond_buffer2_empty, &mutex_buffer2);
        put_item(1, item);
        pthread_cond_signal(&cond_buffer2_full);
        pthread_mutex_unlock(&mutex_buffer2);
    }
    return NULL;
}

int main() {
    pthread_t producer_thread, calculator_thread, customer_thread;

    pthread_mutex_init(&mutex_buffer1, NULL);
    pthread_mutex_init(&mutex_buffer2, NULL);
    pthread_cond_init(&cond_buffer1_empty, NULL);
    pthread_cond_init(&cond_buffer1_full, NULL);
    pthread_cond_init(&cond_buffer2_empty, NULL);
    pthread_cond_init(&cond_buffer2_full, NULL);

    pthread_create(&producer_thread, NULL, producer, NULL);
    pthread_create(&calculator_thread, NULL, calculator, NULL);
    pthread_create(&customer_thread, NULL, customer, NULL);

    pthread_join(producer_thread, NULL);
    pthread_join(calculator_thread, NULL);
    pthread_join(customer_thread, NULL);

    return 0;
}