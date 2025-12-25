#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_READERS 10
#define BUFFER_SIZE 64

char shared_buffer[BUFFER_SIZE];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int running = 1;

void *writer_func(void *arg) {
    (void)arg;
    int counter = 0;

    while (running) {
        pthread_mutex_lock(&mutex);
        counter++;
        snprintf(shared_buffer, BUFFER_SIZE, "%d", counter);
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);

        usleep(500000);
    }

    return NULL;
}

void *reader_func(void *arg) {
    (void)arg;
    char local_buf[BUFFER_SIZE];

    while (running) {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond, &mutex);
        strncpy(local_buf, shared_buffer, BUFFER_SIZE - 1);
        local_buf[BUFFER_SIZE - 1] = '\0';
        pthread_mutex_unlock(&mutex);

        pthread_t tid = pthread_self();
        printf("Reader tid=%lu: buffer = \"%s\"\n",
               (unsigned long)tid, local_buf);
        fflush(stdout);

        usleep(300000 + (rand() % 200000));
    }

    return NULL;
}

int main(void) {
    pthread_t writer;
    pthread_t readers[NUM_READERS];

    srand(time(NULL));

    memset(shared_buffer, 0, BUFFER_SIZE);

    if (pthread_create(&writer, NULL, writer_func, NULL) != 0) {
        perror("pthread_create writer");
        return 1;
    }

    for (int i = 0; i < NUM_READERS; i++) {
        if (pthread_create(&readers[i], NULL, reader_func, NULL) != 0) {
            perror("pthread_create reader");
            return 1;
        }
    }

    sleep(5);

    running = 0;

    pthread_mutex_lock(&mutex);
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);

    pthread_join(writer, NULL);
    for (int i = 0; i < NUM_READERS; i++) {
        pthread_join(readers[i], NULL);
    }

    printf("Done\n");
    return 0;
}
