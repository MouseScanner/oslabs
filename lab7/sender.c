#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#define SHM_KEY 1234
#define SHM_SIZE 256
#define LOCK_FILE "/tmp/lab7_sender.lock"

int check_lock(void) {
    FILE *lock = fopen(LOCK_FILE, "r");
    if (lock != NULL) {
        pid_t pid;
        if (fscanf(lock, "%d", &pid) == 1) {
            if (kill(pid, 0) == 0) {
                fclose(lock);
                return 1;
            }
        }
        fclose(lock);
        unlink(LOCK_FILE);
    }
    return 0;
}

void create_lock(void) {
    FILE *lock = fopen(LOCK_FILE, "w");
    if (lock == NULL) {
        perror("fopen lock");
        exit(EXIT_FAILURE);
    }
    fprintf(lock, "%d\n", getpid());
    fclose(lock);
}

int main(void) {
    if (check_lock()) {
        printf("Sender process already running\n");
        exit(EXIT_FAILURE);
    }

    create_lock();

    int shmid = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        unlink(LOCK_FILE);
        exit(EXIT_FAILURE);
    }

    char *shm_ptr = (char *)shmat(shmid, NULL, 0);
    if (shm_ptr == (char *)-1) {
        perror("shmat");
        shmctl(shmid, IPC_RMID, NULL);
        unlink(LOCK_FILE);
        exit(EXIT_FAILURE);
    }

    while (1) {
        time_t current_time = time(NULL);
        struct tm *time_info = localtime(&current_time);
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", time_info);

        snprintf(shm_ptr, SHM_SIZE, "Time: %s, PID: %d", time_str, getpid());

        sleep(1);
    }

    shmdt(shm_ptr);
    shmctl(shmid, IPC_RMID, NULL);
    unlink(LOCK_FILE);

    return EXIT_SUCCESS;
}
