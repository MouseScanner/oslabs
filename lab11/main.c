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
int version = 0;

void *writer_func(void *arg) {
    (void)arg;
    int counter = 0;

    while (running) {
        if (pthread_mutex_lock(&mutex) != 0) {
            perror("pthread_mutex_lock");
            break;
        }

        counter++;
        snprintf(shared_buffer, BUFFER_SIZE, "%d", counter);
        version++;

        if (pthread_cond_broadcast(&cond) != 0) {
            perror("pthread_cond_broadcast");
            pthread_mutex_unlock(&mutex);
            break;
        }

        if (pthread_mutex_unlock(&mutex) != 0) {
            perror("pthread_mutex_unlock");
            break;
        }

        usleep(500000);
    }

    return NULL;
}

void *reader_func(void *arg) {
    (void)arg;
    char local_buf[BUFFER_SIZE];
    int last_version = 0;

    while (running) {
        if (pthread_mutex_lock(&mutex) != 0) {
            perror("pthread_mutex_lock");
            break;
        }

        while (version == last_version && running) {
            if (pthread_cond_wait(&cond, &mutex) != 0) {
                perror("pthread_cond_wait");
                pthread_mutex_unlock(&mutex);
                return NULL;
            }
        }

        if (running) {
            strncpy(local_buf, shared_buffer, BUFFER_SIZE - 1);
            local_buf[BUFFER_SIZE - 1] = '\0';
            last_version = version;
        }

        if (pthread_mutex_unlock(&mutex) != 0) {
            perror("pthread_mutex_unlock");
            break;
        }

        if (running) {
            pthread_t tid = pthread_self();
            printf("Reader tid=%lu: buffer = \"%s\"\n",
                   (unsigned long)tid, local_buf);
            fflush(stdout);
        }

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

    if (pthread_mutex_lock(&mutex) != 0) {
        perror("pthread_mutex_lock");
    } else {
        version++;
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mutex);
    }

    pthread_join(writer, NULL);
    for (int i = 0; i < NUM_READERS; i++) {
        pthread_join(readers[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    printf("Done\n");
    return 0;
}
