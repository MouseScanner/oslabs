#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

void show_usage(const char *prog) {
    fprintf(stderr, "Usage: %s <mode> <file1> [file2 ...]\n", prog);
    fprintf(stderr, "Modes can be:\n");
    fprintf(stderr, "  Symbolic: [ugoa]*[+-=][rwx]+\n");
    fprintf(stderr, "  Octal: [0-7][0-7][0-7]\n");
    fprintf(stderr, "Examples:\n");
    fprintf(stderr, "  %s +x file.txt\n", prog);
    fprintf(stderr, "  %s u-r file.txt\n", prog);
    fprintf(stderr, "  %s 755 file.txt\n", prog);
}

int process_octal_mode(const char *mode_str, mode_t *result_mode) {
    size_t len = strlen(mode_str);
    if (len != 3) {
        fprintf(stderr, "Octal mode must be 3 digits\n");
        return -1;
    }

    for (size_t i = 0; i < 3; i++) {
        if (mode_str[i] < '0' || mode_str[i] > '7') {
            fprintf(stderr, "Invalid octal digit: %c\n", mode_str[i]);
            return -1;
        }
    }

    *result_mode = 0;
    *result_mode |= (mode_str[0] - '0') << 6;
    *result_mode |= (mode_str[1] - '0') << 3;
    *result_mode |= (mode_str[2] - '0');

    return 0;
}

int process_symbolic_mode(const char *mode_str, mode_t current_mode, mode_t *result_mode) {
    *result_mode = current_mode;

    const char *ptr = mode_str;
    int user_flags = 0;

    while (*ptr && *ptr != '+' && *ptr != '-' && *ptr != '=') {
        switch (*ptr) {
            case 'u': user_flags |= 1; break;
            case 'g': user_flags |= 2; break;
            case 'o': user_flags |= 4; break;
            case 'a': user_flags |= 7; break;
            default:
                fprintf(stderr, "Invalid user specification: %c\n", *ptr);
                return -1;
        }
        ptr++;
    }

    if (user_flags == 0) {
        user_flags = 7;
    }

    if (*ptr != '+' && *ptr != '-' && *ptr != '=') {
        fprintf(stderr, "Expected operator (+, -, =), got: %c\n", *ptr);
        return -1;
    }

    char op = *ptr;
    ptr++;

    int perm_bits = 0;
    while (*ptr) {
        switch (*ptr) {
            case 'r': perm_bits |= 4; break;
            case 'w': perm_bits |= 2; break;
            case 'x': perm_bits |= 1; break;
            default:
                fprintf(stderr, "Invalid permission: %c\n", *ptr);
                return -1;
        }
        ptr++;
    }

    if (user_flags & 1) {
        if (op == '+') {
            *result_mode |= (perm_bits << 6);
        } else if (op == '-') {
            *result_mode &= ~(perm_bits << 6);
        } else if (op == '=') {
            *result_mode &= ~(7 << 6);
            *result_mode |= (perm_bits << 6);
        }
    }

    if (user_flags & 2) {
        if (op == '+') {
            *result_mode |= (perm_bits << 3);
        } else if (op == '-') {
            *result_mode &= ~(perm_bits << 3);
        } else if (op == '=') {
            *result_mode &= ~(7 << 3);
            *result_mode |= (perm_bits << 3);
        }
    }

    if (user_flags & 4) {
        if (op == '+') {
            *result_mode |= perm_bits;
        } else if (op == '-') {
            *result_mode &= ~perm_bits;
        } else if (op == '=') {
            *result_mode &= ~7;
            *result_mode |= perm_bits;
        }
    }

    return 0;
}

int determine_mode_type(const char *mode_str, mode_t current_mode, mode_t *result_mode) {
    int looks_like_octal = 1;
    const char *p = mode_str;

    while (*p) {
        if (*p < '0' || *p > '7') {
            looks_like_octal = 0;
            break;
        }
        p++;
    }

    if (looks_like_octal && strlen(mode_str) == 3) {
        return process_octal_mode(mode_str, result_mode);
    } else {
        return process_symbolic_mode(mode_str, current_mode, result_mode);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        show_usage(argv[0]);
        return 1;
    }

    const char *mode_str = argv[1];
    int processed_count = 0;

    for (int i = 2; i < argc; i++) {
        const char *filepath = argv[i];

        struct stat file_stat;
        if (stat(filepath, &file_stat) == -1) {
            perror(filepath);
            continue;
        }

        mode_t old_mode = file_stat.st_mode;
        mode_t new_mode;

        if (determine_mode_type(mode_str, old_mode, &new_mode) == -1) {
            fprintf(stderr, "Invalid mode: %s\n", mode_str);
            continue;
        }

        if (chmod(filepath, new_mode) == -1) {
            perror(filepath);
            continue;
        }

        printf("Changed mode of '%s' from %04o to %04o\n",
               filepath, old_mode & 0777, new_mode & 0777);
        processed_count++;
    }

    if (processed_count == 0) {
        fprintf(stderr, "No files were successfully processed\n");
        return 1;
    }

    return 0;
}
