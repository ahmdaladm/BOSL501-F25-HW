/*
 * Q2.5 - each thread sums into a LOCAL variable, then merges once
 * into total_sales under the mutex (4 lock operations instead of N).
 * Build: gcc -std=gnu99 -Wall -Wextra -O0 -pthread q2_local.c -o q2_local
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N_SALES   10000000UL
#define N_THREADS 4

/* argument for each thread: its id and the range [start, end) it adds up */
typedef struct {
    int id;
    size_t start, end;
    long long partial;                  /* this thread's own result */
} thread_arg_t;

static int *sales;                      /* shared input, only read by threads */
static long long total_sales;           /* shared counter (global) */
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/* thread function: each thread adds its own part of the array to total_sales */
static void *worker(void *p)
{
    thread_arg_t *a = p;
    long long local = 0;                /* private: lives on this thread's stack */
    for (size_t i = a->start; i < a->end; i++)
        local += sales[i];

    a->partial = local;
    pthread_mutex_lock(&lock);          /* one merge per thread */
    total_sales += local;
    pthread_mutex_unlock(&lock);
    return NULL;
}

/* current time in milliseconds, used to compare how fast each solution is */
static double now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e3 + ts.tv_nsec / 1e6;
}

int main(void)
{
    pthread_t th[N_THREADS];
    thread_arg_t args[N_THREADS];
    size_t chunk = N_SALES / N_THREADS;
    long long expected = 0;
    int rc;

    sales = malloc(N_SALES * sizeof *sales);
    if (!sales) { perror("malloc"); return 1; }
    /* fixed seed: same data every run; 'expected' is the correct single-thread sum */
    srand(42);
    for (size_t i = 0; i < N_SALES; i++) {
        sales[i] = rand() % 1000 + 1;
        expected += sales[i];
    }

    /* split the array into equal parts, one per thread (the last one takes any remainder) */
    double t0 = now_ms();
    for (int t = 0; t < N_THREADS; t++) {
        args[t].id = t;
        args[t].start = t * chunk;
        args[t].end = (t == N_THREADS - 1) ? N_SALES : (t + 1) * chunk;
        args[t].partial = 0;
        if ((rc = pthread_create(&th[t], NULL, worker, &args[t])) != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(rc));
            return 1;
        }
    }
    /* wait for every thread to finish before reading total_sales */
    for (int t = 0; t < N_THREADS; t++)
        pthread_join(th[t], NULL);
    double t1 = now_ms();

    for (int t = 0; t < N_THREADS; t++)
        printf("[local] thread %d partial = %lld\n", t, args[t].partial);
    printf("[local] expected = %lld\n", expected);
    printf("[local] actual   = %lld\n", total_sales);
    printf("[local] result   = %s\n", expected == total_sales ? "OK" : "WRONG");
    printf("[local] time     = %.1f ms\n", t1 - t0);

    pthread_mutex_destroy(&lock);
    free(sales);
    return 0;
}
