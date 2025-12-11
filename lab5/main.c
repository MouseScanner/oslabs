#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>
#include <errno.h>

#define MAX_NAME_LEN 256
#define BUFFER_SIZE 4096

struct file_header {
    char name[MAX_NAME_LEN];             // имя файла (нулевой терминатор)
    off_t size;                         // размер файла
    mode_t mode;                        // права доступа
    uid_t uid;                         // владелец
    gid_t gid;                         // группа
    struct timespec atime;             // время последнего доступа
    struct timespec mtime;             // время последней модификации
};

void print_help() {
    printf("Usage:\n");
    printf("  ./archiver arch_name -i file1     Add file to archive\n");
    printf("  ./archiver arch_name -e file1     Extract file from archive and remove\n");
    printf("  ./archiver arch_name -s            Show archive status\n");
    printf("  ./archiver -h                      Show help\n");
}

int read_header(int fd, struct file_header *hdr) {
    ssize_t n = read(fd, hdr, sizeof(struct file_header));
    if (n == 0) return 0;
    if (n != sizeof(struct file_header)) {
        perror("read header");
        return -1;
    }
    return 1;
}

int write_header(int fd, struct file_header *hdr) {
    ssize_t n = write(fd, hdr, sizeof(struct file_header));
    if (n != sizeof(struct file_header)) {
        perror("write header");
        return -1;
    }
    return 0;
}

int add_file(const char *archive_name, const char *filename) {
    if (access(filename, R_OK) != 0) {
        perror("access input file");
        return 1;
    }

    struct stat st;
    if (stat(filename, &st) < 0) {
        perror("stat input file");
        return 1;
    }

    int a_fd = open(archive_name, O_RDWR | O_CREAT | O_APPEND, 0644);
    if (a_fd < 0) {
        perror("open archive");
        return 1;
    }

    struct file_header hdr = {0};
    strncpy(hdr.name, filename, MAX_NAME_LEN - 1);
    hdr.size = st.st_size;
    hdr.mode = st.st_mode;
    hdr.uid = st.st_uid;
    hdr.gid = st.st_gid;
#ifdef __APPLE__
    hdr.atime = st.st_atimespec;
    hdr.mtime = st.st_mtimespec;
#else
    hdr.atime = st.st_atim;
    hdr.mtime = st.st_mtim;
#endif

    if (write_header(a_fd, &hdr) < 0) {
        close(a_fd);
        return 1;
    }

    int f_fd = open(filename, O_RDONLY);
    if (f_fd < 0) {
        perror("open input file");
        close(a_fd);
        return 1;
    }

    char buffer[BUFFER_SIZE];
    ssize_t rd;
    while ((rd = read(f_fd, buffer, BUFFER_SIZE)) > 0) {
        if (write(a_fd, buffer, rd) != rd) {
            perror("write to archive");
            close(f_fd);
            close(a_fd);
            return 1;
        }
    }
    if (rd < 0) {
        perror("read input file");
        close(f_fd);
        close(a_fd);
        return 1;
    }

    close(f_fd);
    close(a_fd);
    return 0;
}

int extract_file(const char *archive_name, const char *filename) {
    if (access(archive_name, F_OK) != 0) {
        fprintf(stderr, "Archive '%s' does not exist\n", archive_name);
        return 1;
    }

    int a_fd = open(archive_name, O_RDWR);
    if (a_fd < 0) {
        perror("open archive");
        return 1;
    }

    struct stat arch_st;
    if (fstat(a_fd, &arch_st) < 0) {
        perror("fstat archive");
        close(a_fd);
        return 1;
    }

    if (arch_st.st_size == 0) {
        printf("Archive is empty\n");
        close(a_fd);
        return 1;
    }

    char temp_name[] = "./archiver_temp_XXXXXX";
    int temp_fd = mkstemp(temp_name);
    if (temp_fd < 0) {
        perror("create temp file");
        close(a_fd);
        return 1;
    }

    off_t pos = 0;
    int found = 0;
    struct file_header hdr;
    off_t new_archive_size = 0;

    while (pos < arch_st.st_size) {
        ssize_t res = pread(a_fd, &hdr, sizeof(hdr), pos);
        if (res == 0) break;
        if (res != sizeof(hdr)) {
            perror("read header");
            close(a_fd);
            close(temp_fd);
            unlink(temp_name);
            return 1;
        }

        off_t data_pos = pos + sizeof(hdr);
        off_t file_end_pos = data_pos + hdr.size;

        if (strcmp(hdr.name, filename) == 0) {
            int f_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, hdr.mode);
            if (f_fd < 0) {
                perror("create output file");
                close(a_fd);
                close(temp_fd);
                unlink(temp_name);
                return 1;
            }

            off_t to_read = hdr.size;
            char buf[BUFFER_SIZE];
            while (to_read > 0) {
                ssize_t chunk = to_read > BUFFER_SIZE ? BUFFER_SIZE : to_read;
                ssize_t rd = pread(a_fd, buf, chunk, data_pos);
                if (rd <= 0) {
                    perror("read file data");
                    close(f_fd);
                    close(a_fd);
                    close(temp_fd);
                    unlink(temp_name);
                    return 1;
                }
                if (write(f_fd, buf, rd) != rd) {
                    perror("write output file");
                    close(f_fd);
                    close(a_fd);
                    close(temp_fd);
                    unlink(temp_name);
                    return 1;
                }
                data_pos += rd;
                to_read -= rd;
            }
            close(f_fd);

            struct timespec times[2] = {hdr.atime, hdr.mtime};
            if (utimensat(AT_FDCWD, filename, times, 0) < 0) {
                perror("utimensat");
            }
            if (chmod(filename, hdr.mode) < 0) {
                perror("chmod");
            }

            found = 1;
            printf("File '%s' extracted and removed from archive\n", filename);
        } else {
            if (write_header(temp_fd, &hdr) < 0) {
                perror("write header to temp");
                close(a_fd);
                close(temp_fd);
                unlink(temp_name);
                return 1;
            }

            new_archive_size += sizeof(hdr);

            off_t to_read = hdr.size;
            char buf[BUFFER_SIZE];
            while (to_read > 0) {
                ssize_t chunk = to_read > BUFFER_SIZE ? BUFFER_SIZE : to_read;
                ssize_t rd = pread(a_fd, buf, chunk, data_pos);
                if (rd <= 0) {
                    perror("read file data for copy");
                    close(a_fd);
                    close(temp_fd);
                    unlink(temp_name);
                    return 1;
                }
                if (write(temp_fd, buf, rd) != rd) {
                    perror("write to temp archive");
                    close(a_fd);
                    close(temp_fd);
                    unlink(temp_name);
                    return 1;
                }
                data_pos += rd;
                to_read -= rd;
            }

            new_archive_size += hdr.size;
        }

        pos = file_end_pos;
    }

    close(a_fd);
    close(temp_fd);

    if (!found) {
        printf("File '%s' not found in archive.\n", filename);
        unlink(temp_name);
        return 1;
    }

    a_fd = open(archive_name, O_WRONLY | O_TRUNC);
    if (a_fd < 0) {
        perror("open archive for truncate");
        unlink(temp_name);
        return 1;
    }
    close(a_fd);

    if (rename(temp_name, archive_name) < 0) {
        perror("rename temp file");
        a_fd = open(archive_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        temp_fd = open(temp_name, O_RDONLY);
        if (a_fd < 0 || temp_fd < 0) {
            perror("open files for manual copy");
            if (a_fd >= 0) close(a_fd);
            if (temp_fd >= 0) close(temp_fd);
            unlink(temp_name);
            return 1;
        }

        char buf[BUFFER_SIZE];
        ssize_t bytes_read;
        while ((bytes_read = read(temp_fd, buf, BUFFER_SIZE)) > 0) {
            if (write(a_fd, buf, bytes_read) != bytes_read) {
                perror("write to original archive");
                close(a_fd);
                close(temp_fd);
                unlink(temp_name);
                return 1;
            }
        }

        close(a_fd);
        close(temp_fd);
        unlink(temp_name);

        if (bytes_read < 0) {
            perror("read from temp file");
            return 1;
        }
    }

    printf("Archive size updated. New size: %ld bytes\n", (long)new_archive_size);
    return 0;
}

int print_stat(const char *archive_name) {
    if (access(archive_name, F_OK) != 0) {
        fprintf(stderr, "Archive '%s' does not exist\n", archive_name);
        return 1;
    }

    int a_fd = open(archive_name, O_RDONLY);
    if (a_fd < 0) {
        perror("open archive");
        return 1;
    }

    struct stat arch_st;
    if (fstat(a_fd, &arch_st) < 0) {
        perror("fstat archive");
        close(a_fd);
        return 1;
    }

    if (arch_st.st_size == 0) {
        printf("Archive '%s' is empty\n", archive_name);
        close(a_fd);
        return 0;
    }

    off_t pos = 0;
    struct file_header hdr;
    printf("Archive '%s' contents:\n", archive_name);
    printf("%-30s  Size      Mode\n", "Name");
    while (pos < arch_st.st_size) {
        ssize_t res = pread(a_fd, &hdr, sizeof(hdr), pos);
        if (res == 0) break;
        if (res != sizeof(hdr)) {
            perror("read header");
            close(a_fd);
            return 1;
        }
        printf("%-30s  %-8ld  %04o\n", hdr.name, (long)hdr.size, hdr.mode & 0777);
        pos += sizeof(hdr) + hdr.size;
    }
    close(a_fd);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_help();
        return 1;
    }

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_help();
        return 0;
    }

    if (argc < 3) {
        print_help();
        return 1;
    }

    const char *archive_name = argv[1];
    const char *option = argv[2];

    if ((strcmp(option, "-i") == 0 || strcmp(option, "--input") == 0) && argc >= 4) {
        return add_file(archive_name, argv[3]);
    } else if ((strcmp(option, "-e") == 0 || strcmp(option, "--extract") == 0) && argc >= 4) {
        return extract_file(archive_name, argv[3]);
    } else if (strcmp(option, "-s") == 0 || strcmp(option, "--stat") == 0) {
        return print_stat(archive_name);
    } else {
        print_help();
        return 1;
    }
}
