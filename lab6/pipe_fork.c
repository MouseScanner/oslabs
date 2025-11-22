#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#define BUFFER_SIZE 256

int main(void) {
    int pipefd[2];
    pid_t pid;
    char buffer[BUFFER_SIZE];
    time_t parent_time, child_time;
    struct tm *time_info;
    char time_str[64];

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        close(pipefd[0]);

        parent_time = time(NULL);
        time_info = localtime(&parent_time);
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", time_info);

        snprintf(buffer, BUFFER_SIZE, "Time: %s, PID: %d", time_str, getpid());

        if (write(pipefd[1], buffer, strlen(buffer) + 1) == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }

        close(pipefd[1]);

        sleep(5);

        wait(NULL);
    } else {
        close(pipefd[1]);

        sleep(5);

        if (read(pipefd[0], buffer, BUFFER_SIZE) == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        close(pipefd[0]);

        child_time = time(NULL);
        time_info = localtime(&child_time);
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", time_info);

        printf("Child time: %s\n", time_str);
        printf("Child PID: %d\n", getpid());
        printf("Received: %s\n", buffer);
    }

    return EXIT_SUCCESS;
}
