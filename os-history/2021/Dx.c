// 
//
//
//

#include <stdio.h>
#include <pthread.h>
#include <ctype.h>

#define CAPACITY 4      // 缓冲区的最大容量

struct buffer {
    int data[CAPACITY];
    int in;             // 缓冲区的写指针
    int out;            // 缓冲区的读指针
    pthread_mutex_t mutex;
    pthread_cond_t wait_empty_buffer;
    pthread_cond_t wait_full_buffer;
};

void buffer_init(struct buffer *buffer)
{
    buffer->in = 0;
    buffer->out = 0;
    pthread_mutex_init(&buffer->mutex, NULL);
    pthread_cond_init(&buffer->wait_empty_buffer, NULL);
    pthread_cond_init(&buffer->wait_full_buffer, NULL);
}

// 判断缓冲区是否为空
int buffer_is_empty(struct buffer *buffer)
{
    return buffer->in == buffer->out;
}

// 判断缓冲区是否为满
int buffer_is_full(struct buffer *buffer)
{
    return (buffer->in + 1) % CAPACITY == buffer->out;
}

// 向缓冲区中追加一个数据
void buffer_put(struct buffer *buffer, int item)
{
    pthread_mutex_lock(&buffer->mutex);
    while (buffer_is_full(buffer))
        pthread_cond_wait(&buffer->wait_empty_buffer, &buffer->mutex);

    buffer->data[buffer->in] = item;
    buffer->in = (buffer->in + 1) % CAPACITY;

    pthread_cond_signal(&buffer->wait_full_buffer);
    pthread_mutex_unlock(&buffer->mutex);
}

// 从缓冲区中取走一个数据
int buffer_get(struct buffer *buffer)
{
    int item;

    pthread_mutex_lock(&buffer->mutex);
    while (buffer_is_empty(buffer))
        pthread_cond_wait(&buffer->wait_full_buffer, &buffer->mutex);

    item = buffer->data[buffer->out];
    buffer->out = (buffer->out + 1) % CAPACITY;

    pthread_cond_signal(&buffer->wait_empty_buffer);
    pthread_mutex_unlock(&buffer->mutex);
    return item;
}

#define ITEM_COUNT (2 * CAPACITY)

struct buffer buffer_pc, buffer_cc;

void *produce(void *arg)
{
    int i;
    int item;

    for (i = 0; i < ITEM_COUNT; i++) { 
        item = 'a' + i;
        printf("produce item: %c\n", item);
        buffer_put(&buffer_pc, item);
    }
    return NULL;
}

void *compute(void *arg)
{
    int i;
    int item;

    for (i = 0; i < ITEM_COUNT; i++) { 
        item = buffer_get(&buffer_pc); 
        item = toupper(item) + 1;
        printf("    compute item: %c\n", item); 
        buffer_put(&buffer_cc, item);
    }
    return NULL;
}

void *consume(void *arg)
{
    int i;
    int item;

    for (i = 0; i < ITEM_COUNT; i++) { 
        item = buffer_get(&buffer_cc); 
        printf("        consume item: %c\n", item); 
    }
    return NULL;
}

int main()
{ 
    buffer_init(&buffer_pc);
    buffer_init(&buffer_cc);

    pthread_t producer_tid;
    pthread_create(&producer_tid, NULL, produce, NULL);

    pthread_t computer_tid;
    pthread_create(&computer_tid, NULL, compute, NULL);

    consume(NULL); 
    return 0;
}
