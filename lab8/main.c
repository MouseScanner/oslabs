#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_READERS 10
#define BUFFER_SIZE 64

char shared_buffer[BUFFER_SIZE];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int running = 1;

void *writer_func(void *arg) {
    (void)arg;
    int counter = 0;

    while (running) {
        pthread_mutex_lock(&mutex);

        snprintf(shared_buffer, BUFFER_SIZE, "%d", counter);
        counter++;

        pthread_mutex_unlock(&mutex);

        usleep(500000);
    }

    return NULL;
}

void *reader_func(void *arg) {
    int id = *(int *)arg;
    char local_buf[BUFFER_SIZE];

    while (running) {
        pthread_mutex_lock(&mutex);

        strncpy(local_buf, shared_buffer, BUFFER_SIZE - 1);
        local_buf[BUFFER_SIZE - 1] = '\0';

        pthread_mutex_unlock(&mutex);

        printf("Reader %d (tid=%lu): buffer = \"%s\"\n",
               id, (unsigned long)pthread_self(), local_buf);
        fflush(stdout);

        usleep(300000 + (rand() % 200000));
    }

    return NULL;
}

int main(void) {
    pthread_t writer;
    pthread_t readers[NUM_READERS];
    int reader_ids[NUM_READERS];

    srand(time(NULL));

    memset(shared_buffer, 0, BUFFER_SIZE);

    if (pthread_create(&writer, NULL, writer_func, NULL) != 0) {
        perror("pthread_create writer");
        return 1;
    }

    for (int i = 0; i < NUM_READERS; i++) {
        reader_ids[i] = i + 1;
        if (pthread_create(&readers[i], NULL, reader_func, &reader_ids[i]) != 0) {
            perror("pthread_create reader");
            return 1;
        }
    }

    sleep(5);

    running = 0;

    pthread_join(writer, NULL);
    for (int i = 0; i < NUM_READERS; i++) {
        pthread_join(readers[i], NULL);
    }

    pthread_mutex_destroy(&mutex);

    printf("Done\n");
    return 0;
}
