
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
} thread_arg_t;
static int *sales;                      /* shared (read-only) input   */
static volatile long long total_sales;
/* thread function: each thread adds its own part of the array to total_sales */
static void *worker(void *p)
{
    thread_arg_t *a = p;
    for (size_t i = a->start; i < a->end; i++)
        total_sales += sales[i];        /* RACE CONDITION HERE: load, add, store are not atomic */
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
        if ((rc = pthread_create(&th[t], NULL, worker, &args[t])) != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(rc));
            return 1;
        }
    }
    /* wait for every thread to finish before reading total_sales */
    for (int t = 0; t < N_THREADS; t++)
        pthread_join(th[t], NULL);
    double t1 = now_ms();

    printf("[race]  expected = %lld\n", expected);
    printf("[race]  actual   = %lld\n", total_sales);
    printf("[race]  lost     = %lld  (%s)\n", expected - total_sales,
           expected == total_sales ? "OK" : "WRONG - race condition");
    printf("[race]  time     = %.1f ms\n", t1 - t0);

    free(sales);
    return 0;
}
