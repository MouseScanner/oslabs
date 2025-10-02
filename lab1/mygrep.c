#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s pattern [file...]\n", argv[0]);
        return 1;
    }

    char *pattern = argv[1];

    // если файлы не указаны, читаем из stdin
    if (argc == 2) {
        char line[4096];
        while (fgets(line, sizeof(line), stdin)) {
            if (strstr(line, pattern)) {
                printf("%s", line);
            }
        }
        return 0;
    }

    // обрабатываем указанные файлы
    for (int i = 2; i < argc; i++) {
        FILE *file = fopen(argv[i], "r");
        if (!file) {
            perror(argv[i]);
            continue;
        }

        char line[4096];
        int multiple_files = (argc > 3);

        while (fgets(line, sizeof(line), file)) {
            if (strstr(line, pattern)) {
                if (multiple_files) {
                    printf("%s:", argv[i]);
                }
                printf("%s", line);
            }
        }

        fclose(file);
    }

    return 0;
}
