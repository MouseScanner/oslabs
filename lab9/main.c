#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define BUFFER_SIZE 64

char shared_buffer[BUFFER_SIZE];
sem_t sem;
int running = 1;

void *writer_func(void *arg) {
    (void)arg;
    int counter = 0;

    while (running) {
        counter++;
        snprintf(shared_buffer, BUFFER_SIZE, "%d", counter);
        sem_post(&sem);
        sleep(1);
    }
    return NULL;
}

void *reader_func(void *arg) {
    (void)arg;
    char local_buf[BUFFER_SIZE];

    while (running) {
        sem_wait(&sem);
        strncpy(local_buf, shared_buffer, BUFFER_SIZE - 1);
        local_buf[BUFFER_SIZE - 1] = '\0';

        pthread_t tid = pthread_self();
        printf("Reader tid=%lu: buffer = \"%s\"\n",
               (unsigned long)tid, local_buf);
        fflush(stdout);
    }
    return NULL;
}

int main(void) {
    pthread_t writer;
    pthread_t reader;

    memset(shared_buffer, 0, BUFFER_SIZE);

    if (sem_init(&sem, 0, 0) != 0) {
        perror("sem_init");
        return 1;
    }

    if (pthread_create(&writer, NULL, writer_func, NULL) != 0) {
        perror("pthread_create writer");
        sem_destroy(&sem);
        return 1;
    }

    if (pthread_create(&reader, NULL, reader_func, NULL) != 0) {
        perror("pthread_create reader");
        running = 0;
        sem_post(&sem);
        pthread_join(writer, NULL);
        sem_destroy(&sem);
        return 1;
    }

    sleep(5);

    running = 0;
    sem_post(&sem);

    pthread_join(writer, NULL);
    pthread_join(reader, NULL);

    sem_destroy(&sem);

    printf("Done\n");
    return 0;
}
