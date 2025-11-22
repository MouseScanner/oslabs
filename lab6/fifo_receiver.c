#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#define FIFO_NAME "/tmp/lab6_fifo"
#define BUFFER_SIZE 256

int main(void) {
    int fd;
    char buffer[BUFFER_SIZE];
    time_t current_time;
    struct tm *time_info;
    char time_str[64];

    fd = open(FIFO_NAME, O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    sleep(10);

    if (read(fd, buffer, BUFFER_SIZE) == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }

    close(fd);

    current_time = time(NULL);
    time_info = localtime(&current_time);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", time_info);

    printf("Child time: %s\n", time_str);
    printf("Child PID: %d\n", getpid());
    printf("Received: %s\n", buffer);

    return EXIT_SUCCESS;
}
