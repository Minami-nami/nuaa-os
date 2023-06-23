#include "coro.h"
#include <stdio.h>
#include <stdlib.h>

#define LEADING_SPACES_PRODUCE ""
#define LEADING_SPACES_FILTER "        "
#define LEADING_SPACES_CONSUME "                "
#define N 8

coro_t *co_produce;
coro_t *co_filter;
coro_t *co_consume;

void produce();
void filter();
void consume();

void produce() {
    int i;
    for (i = 0; i < N; ++i) {
        printf("produce %d\n", i);
        coro_yield(i);
    }
}

void filter() {
    int i;
    for (i = 0; i < N; ++i) {
        int value = coro_resume(co_produce);
        printf(LEADING_SPACES_FILTER "filter %d\n", value);
        coro_yield(value * 10);
    }
}

void consume() {
    int i;
    for (i = 0; i < N; ++i) {
        int value = coro_resume(co_filter);
        printf(LEADING_SPACES_CONSUME "consume %d\n", value);
    }
    puts(LEADING_SPACES_CONSUME "END");
    exit(EXIT_SUCCESS);
}

int main() {
    puts(LEADING_SPACES_CONSUME "CONSUME");
    puts(LEADING_SPACES_FILTER "FILTER");
    puts(LEADING_SPACES_PRODUCE "PRODUCE");
    co_produce = coro_new(produce);
    co_filter  = coro_new(filter);
    co_consume = coro_new(consume);
    coro_boot(co_consume);
    return 0;
}
