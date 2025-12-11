#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>

#define SHM_KEY 1234
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

    while (1) {
        time_t current_time = time(NULL);
        struct tm *time_info = localtime(&current_time);
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", time_info);

        printf("Receiver time: %s\n", time_str);
        printf("Receiver PID: %d\n", getpid());
        printf("Received: %s\n\n", shm_ptr);
        fflush(stdout);

        sleep(1);
    }

    shmdt(shm_ptr);

    return EXIT_SUCCESS;
}
