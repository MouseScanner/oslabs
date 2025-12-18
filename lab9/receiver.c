#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>

#define SHM_KEY 5678
#define SEM_KEY 5679
#define SHM_SIZE 256

int main(void) {
    int shmid = shmget(SHM_KEY, SHM_SIZE, 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    char *shm_ptr = (char *)shmat(shmid, NULL, 0);
    if (shm_ptr == (char *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    int semid = semget(SEM_KEY, 1, 0666);
    if (semid == -1) {
        perror("semget");
        shmdt(shm_ptr);
        exit(EXIT_FAILURE);
    }

    struct sembuf sem_op;
    sem_op.sem_num = 0;
    sem_op.sem_flg = 0;

    while (1) {
        sem_op.sem_op = -1;
        if (semop(semid, &sem_op, 1) == -1) {
            perror("semop wait");
            break;
        }

        time_t current_time = time(NULL);
        struct tm *time_info = localtime(&current_time);
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", time_info);

        char received[SHM_SIZE];
        strncpy(received, shm_ptr, SHM_SIZE - 1);
        received[SHM_SIZE - 1] = '\0';

        sem_op.sem_op = 1;
        if (semop(semid, &sem_op, 1) == -1) {
            perror("semop post");
            break;
        }

        printf("Receiver time: %s\n", time_str);
        printf("Receiver PID: %d\n", getpid());
        printf("Received string: %s\n\n", received);
        fflush(stdout);

        usleep(500000);
    }

    shmdt(shm_ptr);

    return EXIT_SUCCESS;
}
