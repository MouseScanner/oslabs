#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#define ARCHIVE_MAGIC "ARCHV001"
#define MAX_FILENAME 256

typedef struct {
    char magic[8];          // сигнатура архива
    char filename[MAX_FILENAME];  // имя файла
    mode_t mode;            // права доступа
    uid_t uid;             // владелец
    gid_t gid;             // группа
    time_t mtime;          // время модификации
    off_t size;            // размер файла
    off_t offset;          // смещение в архиве
} ArchiveEntry;

void show_help(void) {
    printf("Простой архиватор\n");
    printf("Использование:\n");
    printf("  %s arch_name -i file1 [file2 ...]  - добавить файлы в архив\n", "archiver");
    printf("  %s arch_name -e file1              - извлечь файл из архива\n", "archiver");
    printf("  %s arch_name -s                    - показать содержимое архива\n", "archiver");
    printf("  %s -h                               - показать эту справку\n", "archiver");
}

int write_entry_header(int fd, ArchiveEntry *entry) {
    if (write(fd, entry, sizeof(ArchiveEntry)) != sizeof(ArchiveEntry)) {
        perror("write header");
        return -1;
    }
    return 0;
}

int read_entry_header(int fd, ArchiveEntry *entry) {
    ssize_t bytes_read = read(fd, entry, sizeof(ArchiveEntry));
    if (bytes_read == 0) return 0; // конец файла
    if (bytes_read != sizeof(ArchiveEntry)) {
        perror("read header");
        return -1;
    }
    return 1;
}

int add_file_to_archive(const char *arch_name, const char *filename) {
    struct stat st;
    if (stat(filename, &st) == -1) {
        perror(filename);
        return -1;
    }

    int arch_fd = open(arch_name, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (arch_fd == -1) {
        perror(arch_name);
        return -1;
    }

    int file_fd = open(filename, O_RDONLY);
    if (file_fd == -1) {
        perror(filename);
        close(arch_fd);
        return -1;
    }

    // создаем запись для архива
    ArchiveEntry entry;
    memset(&entry, 0, sizeof(entry));
    memcpy(entry.magic, ARCHIVE_MAGIC, 8);
    strncpy(entry.filename, filename, MAX_FILENAME - 1);
    entry.filename[MAX_FILENAME - 1] = '\0';
    entry.mode = st.st_mode;
    entry.uid = st.st_uid;
    entry.gid = st.st_gid;
    entry.mtime = st.st_mtime;
    entry.size = st.st_size;
    entry.offset = lseek(arch_fd, 0, SEEK_CUR) + sizeof(ArchiveEntry);

    // записываем заголовок
    if (write_entry_header(arch_fd, &entry) == -1) {
        close(file_fd);
        close(arch_fd);
        return -1;
    }

    // копируем содержимое файла
    char buffer[4096];
    ssize_t bytes_read;
    while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
        if (write(arch_fd, buffer, bytes_read) != bytes_read) {
            perror("write file data");
            close(file_fd);
            close(arch_fd);
            return -1;
        }
    }

    if (bytes_read == -1) {
        perror("read file");
        close(file_fd);
        close(arch_fd);
        return -1;
    }

    close(file_fd);
    close(arch_fd);
    
    printf("Добавлен файл: %s\n", filename);
    return 0;
}

int extract_file_from_archive(const char *arch_name, const char *filename) {
    int arch_fd = open(arch_name, O_RDONLY);
    if (arch_fd == -1) {
        perror(arch_name);
        return -1;
    }

    ArchiveEntry entry;
    off_t found_offset = -1;
    off_t found_size = 0;
    mode_t found_mode = 0;
    uid_t found_uid = 0;
    gid_t found_gid = 0;
    time_t found_mtime = 0;

    // ищем файл в архиве
    while (read_entry_header(arch_fd, &entry) > 0) {
        if (memcmp(entry.magic, ARCHIVE_MAGIC, 8) != 0) {
            fprintf(stderr, "Неверная сигнатура архива\n");
            close(arch_fd);
            return -1;
        }

        if (strcmp(entry.filename, filename) == 0) {
            found_offset = entry.offset;
            found_size = entry.size;
            found_mode = entry.mode;
            found_uid = entry.uid;
            found_gid = entry.gid;
            found_mtime = entry.mtime;
            break;
        }

        // пропускаем данные файла
        lseek(arch_fd, entry.size, SEEK_CUR);
    }

    if (found_offset == -1) {
        printf("Файл %s не найден в архиве\n", filename);
        close(arch_fd);
        return -1;
    }

    // создаем файл
    int file_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, found_mode);
    if (file_fd == -1) {
        perror(filename);
        close(arch_fd);
        return -1;
    }

    // копируем данные
    lseek(arch_fd, found_offset, SEEK_SET);
    char buffer[4096];
    off_t remaining = found_size;
    
    while (remaining > 0) {
        size_t to_read = (remaining > sizeof(buffer)) ? sizeof(buffer) : (size_t)remaining;
        ssize_t bytes_read = read(arch_fd, buffer, to_read);
        
        if (bytes_read <= 0) break;
        
        if (write(file_fd, buffer, bytes_read) != bytes_read) {
            perror("write extracted file");
            close(file_fd);
            close(arch_fd);
            return -1;
        }
        
        remaining -= bytes_read;
    }

    // восстанавливаем атрибуты
    if (fchown(file_fd, found_uid, found_gid) == -1) {
        perror("fchown");
    }
    
    struct timespec times[2];
    times[0].tv_sec = found_mtime;
    times[0].tv_nsec = 0;
    times[1].tv_sec = found_mtime;
    times[1].tv_nsec = 0;
    
    if (futimens(file_fd, times) == -1) {
        perror("futimens");
    }

    close(file_fd);
    close(arch_fd);
    
    printf("Извлечен файл: %s\n", filename);
    return 0;
}

int show_archive_status(const char *arch_name) {
    int arch_fd = open(arch_name, O_RDONLY);
    if (arch_fd == -1) {
        perror(arch_name);
        return -1;
    }

    printf("Содержимое архива: %s\n", arch_name);
    printf("%-20s %10s %10s %s\n", "Имя файла", "Размер", "Права", "Дата");
    printf("%-20s %10s %10s %s\n", "----------", "------", "------", "----");

    ArchiveEntry entry;
    while (read_entry_header(arch_fd, &entry) > 0) {
        if (memcmp(entry.magic, ARCHIVE_MAGIC, 8) != 0) {
            fprintf(stderr, "Неверная сигнатура архива\n");
            close(arch_fd);
            return -1;
        }

        char time_str[32];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", localtime(&entry.mtime));
        
        printf("%-20s %10ld %10o %s\n", 
               entry.filename, 
               (long)entry.size, 
               (unsigned int)entry.mode & 0777,
               time_str);

        // пропускаем данные файла
        lseek(arch_fd, entry.size, SEEK_CUR);
    }

    close(arch_fd);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        show_help();
        return 1;
    }

    // проверяем флаг помощи
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        show_help();
        return 0;
    }

    if (argc < 3) {
        fprintf(stderr, "Недостаточно аргументов\n");
        show_help();
        return 1;
    }

    const char *arch_name = argv[1];
    const char *operation = argv[2];

    if (strcmp(operation, "-i") == 0 || strcmp(operation, "--input") == 0) {
        // добавление файлов
        if (argc < 4) {
            fprintf(stderr, "Не указаны файлы для добавления\n");
            return 1;
        }
        
        for (int i = 3; i < argc; i++) {
            if (add_file_to_archive(arch_name, argv[i]) == -1) {
                return 1;
            }
        }
    }
    else if (strcmp(operation, "-e") == 0 || strcmp(operation, "--extract") == 0) {
        // извлечение файла
        if (argc != 4) {
            fprintf(stderr, "Укажите один файл для извлечения\n");
            return 1;
        }
        
        return extract_file_from_archive(arch_name, argv[3]);
    }
    else if (strcmp(operation, "-s") == 0 || strcmp(operation, "--stat") == 0) {
        // показ статуса
        return show_archive_status(arch_name);
    }
    else {
        fprintf(stderr, "Неизвестная операция: %s\n", operation);
        show_help();
        return 1;
    }

    return 0;
}
