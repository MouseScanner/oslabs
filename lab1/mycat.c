#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    int show_line_numbers = 0;
    int number_nonblank = 0;
    int show_ends = 0;
    int file_start_idx = 1;
    
    // парсим флаги
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            for (int j = 1; argv[i][j]; j++) {
                switch (argv[i][j]) {
                    case 'n': show_line_numbers = 1; break;
                    case 'b': number_nonblank = 1; break;
                    case 'E': show_ends = 1; break;
                }
            }
            file_start_idx = i + 1;
        } else {
            break;
        }
    }
    
    if (file_start_idx >= argc) {
        fprintf(stderr, "Usage: %s [-nbE] file...\n", argv[0]);
        return 1;
    }
    
    // обрабатываем каждый файл
    for (int f = file_start_idx; f < argc; f++) {
        FILE *file = fopen(argv[f], "r");
        if (!file) {
            perror(argv[f]);
            continue;
        }
        
        char line[4096];
        int line_num = 1;
        int nonblank_num = 1;
        
        while (fgets(line, sizeof(line), file)) {
            int is_blank = (line[0] == '\n');
            
            // нумерация строк
            if (show_line_numbers && !number_nonblank) {
                printf("%6d\t", line_num);
            } else if (number_nonblank && !is_blank) {
                printf("%6d\t", nonblank_num++);
            } else if (number_nonblank && is_blank) {
                printf("      \t");
            }
            
            // выводим строку
            int len = strlen(line);
            if (show_ends && len > 0 && line[len-1] == '\n') {
                line[len-1] = '\0';
                printf("%s$\n", line);
            } else {
                printf("%s", line);
            }
            
            line_num++;
        }
        
        fclose(file);
    }
    
    return 0;
}
