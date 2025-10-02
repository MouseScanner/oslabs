#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

// вызывается при выходе из программы
void exit_handler(void) {
    printf("exit handler called, pid=%d\n", getpid());
}

// обработчик для Ctrl+C
void sigint_handler(int sig) {
    printf("\nSIGINT received in process %d (signal %d)\n", getpid(), sig);
    exit(130);
}

// обработчик для kill -TERM
void sigterm_handler(int sig) {
    printf("\nSIGTERM received in process %d (signal %d)\n", getpid(), sig);
    exit(143);
}

int main(void) {
    pid_t pid;
    int status;

    // регистрируем функию которая вызовется при exit()
    if (atexit(exit_handler) != 0) {
        perror("atexit");
        return EXIT_FAILURE;
    }

    // устанавливаем обработчик sigint через signal
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("signal");
        return EXIT_FAILURE;
    }

    // устанавливаем обработчик sigterm через sigaction
    // sigterm нужен для завершения процесса
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigterm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    printf("Parent started: pid=%d ppid=%d\n", getpid(), getppid());

    // создаем дочерний процесс
    pid = fork();

    if (pid == -1) {
        // ошибка fork
        perror("fork");
        return EXIT_FAILURE;
    }
    else if (pid == 0) {
        // код дочернего процесса (pid == 0)
        printf("Child process: pid=%d ppid=%d\n", getpid(), getppid());
        printf("Child working...\n");
        sleep(3);
        printf("Child done\n");
        exit(99);  // завершаем дочерний процесс с кодом 99
    }
    else {
        // код родительского процесса (pid > 0)
        printf("Parent created child with pid=%d\n", pid);
        printf("Parent waiting...\n");

        // ждем завершения дочернего процесса
        if (wait(&status) == -1) {
            perror("wait");
            return EXIT_FAILURE;
        }

        // проверяем как завершился дочерний процесс
        if (WIFEXITED(status)) {
            // нормальное завершение через exit()
            printf("Child exited normally with code %d\n", WEXITSTATUS(status));
        } 
        else if (WIFSIGNALED(status)) {
            // завершение по сигналу
            printf("Child terminated by signal %d\n", WTERMSIG(status));
        }
        else {
            printf("Child status unknown\n");
        }

        printf("Parent done\n");
    }

    return 0;
}
