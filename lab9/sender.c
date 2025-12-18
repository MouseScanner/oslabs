#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>
#include <errno.h>

#define SHM_KEY 5678
#define SEM_KEY 5679
#define SHM_SIZE 256

#if defined(__linux__)
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};
#endif

int main(void) {
    int shmid = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    char *shm_ptr = (char *)shmat(shmid, NULL, 0);
    if (shm_ptr == (char *)-1) {
        perror("shmat");
        shmctl(shmid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }

    int semid = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget");
        shmdt(shm_ptr);
        shmctl(shmid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }

    union semun sem_arg;
    sem_arg.val = 1;
    if (semctl(semid, 0, SETVAL, sem_arg) == -1) {
        perror("semctl SETVAL");
        shmdt(shm_ptr);
        shmctl(shmid, IPC_RMID, NULL);
        semctl(semid, 0, IPC_RMID);
        exit(EXIT_FAILURE);
    }

    struct sembuf sem_op;
    sem_op.sem_num = 0;
    sem_op.sem_flg = 0;

    while (1) {
        time_t current_time = time(NULL);
        struct tm *time_info = localtime(&current_time);
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", time_info);

        sem_op.sem_op = -1;
        if (semop(semid, &sem_op, 1) == -1) {
            perror("semop wait");
            break;
        }

        snprintf(shm_ptr, SHM_SIZE, "Time: %s, PID: %d", time_str, getpid());

        sem_op.sem_op = 1;
        if (semop(semid, &sem_op, 1) == -1) {
            perror("semop post");
            break;
        }

        sleep(3);
    }

    shmdt(shm_ptr);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);

    return EXIT_SUCCESS;
}
