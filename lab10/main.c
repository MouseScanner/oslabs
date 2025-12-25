#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_READERS 10
#define BUFFER_SIZE 64

char shared_buffer[BUFFER_SIZE];
pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
int running = 1;

void *writer_func(void *arg) {
    (void)arg;
    int counter = 0;

    while (running) {
        if (pthread_rwlock_wrlock(&rwlock) != 0) {
            perror("pthread_rwlock_wrlock");
            break;
        }

        counter++;
        snprintf(shared_buffer, BUFFER_SIZE, "%d", counter);

        if (pthread_rwlock_unlock(&rwlock) != 0) {
            perror("pthread_rwlock_unlock");
            break;
        }

        usleep(500000);
    }

    return NULL;
}

void *reader_func(void *arg) {
    (void)arg;
    char local_buf[BUFFER_SIZE];

    while (running) {
        if (pthread_rwlock_rdlock(&rwlock) != 0) {
            perror("pthread_rwlock_rdlock");
            break;
        }

        strncpy(local_buf, shared_buffer, BUFFER_SIZE - 1);
        local_buf[BUFFER_SIZE - 1] = '\0';

        if (pthread_rwlock_unlock(&rwlock) != 0) {
            perror("pthread_rwlock_unlock");
            break;
        }

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

    pthread_join(writer, NULL);
    for (int i = 0; i < NUM_READERS; i++) {
        pthread_join(readers[i], NULL);
    }

    printf("Done\n");
    return 0;
}
