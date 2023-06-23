
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>

sem_t sem1, sem2;

pthread_t T1, T2, T3, T4;

void *T1_entry(void *arg) {
    sleep(2);  // 睡眠2秒，不准删除此条语句，否则答题无效
    puts("T1");
    sem_post(&sem1);
    sem_post(&sem1);
    return NULL;
}

void *T2_entry(void *arg) {
    sleep(1);  // 睡眠1秒，不准删除此条语句，否则答题无效
    sem_wait(&sem1);
    puts("T2");
    sem_post(&sem2);
    return NULL;
}

void *T3_entry(void *arg) {
    sleep(1);  // 睡眠1秒，不准删除此条语句，否则答题无效
    sem_wait(&sem1);
    puts("T3");
    sem_post(&sem2);
    return NULL;
}

void *T4_entry(void *arg) {
    sem_wait(&sem2);
    sem_wait(&sem2);
    puts("T4");

    return NULL;
}

int main() {
    sem_init(&sem1, 0, 0);
    sem_init(&sem2, 0, 0);
    pthread_create(&T1, NULL, T1_entry, NULL);
    pthread_create(&T2, NULL, T2_entry, NULL);
    pthread_create(&T3, NULL, T3_entry, NULL);
    pthread_create(&T4, NULL, T4_entry, NULL);
    pthread_join(T1, NULL);
    pthread_join(T2, NULL);
    pthread_join(T3, NULL);
    pthread_join(T4, NULL);
}