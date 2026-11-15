/*
 * Q2.4 (comparison) - fix the race with a binary POSIX semaphore.
 * Build: gcc -std=gnu99 -Wall -Wextra -O0 -pthread q2_sem.c -o q2_sem
 */
#include <pthread.h>
#include <semaphore.h>
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

static int *sales;                      /* shared input, only read by threads */
static long long total_sales;           /* shared counter (global) */
static sem_t sem;                       /* binary semaphore, initial value 1 */

/* thread function: each thread adds its own part of the array to total_sales */
static void *worker(void *p)
{
    thread_arg_t *a = p;
    for (size_t i = a->start; i < a->end; i++) {
        sem_wait(&sem);                 /* P(): 1 -> 0, others block */
        total_sales += sales[i];
        sem_post(&sem);                 /* V(): 0 -> 1 */
    }
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

    if (sem_init(&sem, 0, 1) != 0) { perror("sem_init"); return 1; }

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

    printf("[sem]   expected = %lld\n", expected);
    printf("[sem]   actual   = %lld\n", total_sales);
    printf("[sem]   result   = %s\n", expected == total_sales ? "OK" : "WRONG");
    printf("[sem]   time     = %.1f ms\n", t1 - t0);

    sem_destroy(&sem);
    free(sales);
    return 0;
}
