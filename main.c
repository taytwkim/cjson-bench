#define _POSIX_C_SOURCE 200809L
#include "cjson/cJSON.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct line {
    char *text;
    size_t length;
};

struct workload {
    struct line *lines;
    size_t count;
    size_t repetitions;
};

struct workload work;

struct startup {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    size_t ready;
    int start;
};

struct startup startup = {
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .condition = PTHREAD_COND_INITIALIZER
};

struct worker {
    size_t failed_line;
    struct timespec end;
};

void load_file(const char *filename)
{
    FILE *file = fopen(filename, "r");

    if (!file) {
        perror(filename);
        exit(EXIT_FAILURE);
    }

    char *text = NULL;
    size_t capacity = 0;
    ssize_t length;

    while ((length = getline(&text, &capacity, file)) != -1) {
        if (memchr(text, '\0', length)) {
            fprintf(stderr, "Input contains a NUL byte\n");
            exit(EXIT_FAILURE);
        }

        struct line *lines = realloc(work.lines, (work.count + 1) * sizeof(*lines));

        if (!lines) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }

        work.lines = lines;
        work.lines[work.count].text = text;
        work.lines[work.count].length = length + 1;
        work.count++;

        text = NULL;
        capacity = 0;
    }

    free(text);

    if (!feof(file)) {
        fprintf(stderr, "Could not read the file\n");
        exit(EXIT_FAILURE);
    }

    fclose(file);

    if (work.count == 0) {
        fprintf(stderr, "Input file is empty\n");
        exit(EXIT_FAILURE);
    }
}

void *run(void *argument)
{
    struct worker *worker = argument;

    pthread_mutex_lock(&startup.mutex);

    startup.ready++;

    pthread_cond_broadcast(&startup.condition);

    while (!startup.start) {
        pthread_cond_wait(&startup.condition, &startup.mutex);
    }

    pthread_mutex_unlock(&startup.mutex);

    for (size_t repeat = 0; repeat < work.repetitions; repeat++) {
        for (size_t i = 0; i < work.count; i++) {
            cJSON *object = cJSON_ParseWithLengthOpts(
                work.lines[i].text, work.lines[i].length, NULL, 1);

            if (object == NULL) {
                worker->failed_line = i + 1;
                return NULL;
            }

            cJSON_Delete(object);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &worker->end);
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 3 && argc != 4) {
        fprintf(stderr, "Usage: %s FILE.jsonl THREADS [REPETITIONS]\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *end;

    errno = 0;
    long thread_count = strtol(argv[2], &end, 10);

    if (errno || *end || thread_count <= 0) {
        fprintf(stderr, "Thread count must be a positive integer\n");
        return EXIT_FAILURE;
    }

    long repetitions = 1;

    if (argc == 4) {
        errno = 0;
        repetitions = strtol(argv[3], &end, 10);

        if (errno || *end || repetitions <= 0) {
            fprintf(stderr, "Repetitions must be a positive integer\n");
            return EXIT_FAILURE;
        }
    }

    work.repetitions = repetitions;

    load_file(argv[1]);

    pthread_t *threads = calloc(thread_count, sizeof(*threads));
    struct worker *workers = calloc(thread_count, sizeof(*workers));

    if (!threads || !workers) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    for (long i = 0; i < thread_count; i++) {
        int error = pthread_create(&threads[i], NULL, run, &workers[i]);

        if (error) {
            fprintf(stderr, "pthread_create: %s\n", strerror(error));
            exit(EXIT_FAILURE);
        }
    }

    struct timespec start;

    pthread_mutex_lock(&startup.mutex);

    while (startup.ready < (size_t)thread_count) {
        pthread_cond_wait(&startup.condition, &startup.mutex);
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    startup.start = 1;

    pthread_cond_broadcast(&startup.condition);
    
    pthread_mutex_unlock(&startup.mutex);

    for (long i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    double seconds = 0;

    for (long i = 0; i < thread_count; i++) {
        if (workers[i].failed_line) {
            fprintf(stderr, "Thread %ld: parse failed on line %zu\n",
                    i + 1, workers[i].failed_line);
            exit(EXIT_FAILURE);
        }

        double elapsed = (double)(workers[i].end.tv_sec - start.tv_sec) +
                         (workers[i].end.tv_nsec - start.tv_nsec) / 1e9;

        if (elapsed > seconds) {
            seconds = elapsed;
        }
    }

    double total = (double)work.count * work.repetitions * thread_count;

    double throughput = 0;

    if (seconds > 0) {
        throughput = total / seconds;
    }

    printf("Threads: %ld\n", thread_count);
    printf("Records per pass: %zu\n", work.count);
    printf("Repetitions per thread: %zu\n", work.repetitions);
    printf("Total records: %.0f\n", total);
    printf("Elapsed: %.6f s\n", seconds);
    printf("Throughput: %.0f records/s\n", throughput);

    free(workers);
    free(threads);

    for (size_t i = 0; i < work.count; i++) {
        free(work.lines[i].text);
    }

    free(work.lines);

    pthread_cond_destroy(&startup.condition);
    pthread_mutex_destroy(&startup.mutex);

    return EXIT_SUCCESS;
}
