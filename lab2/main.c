#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <unistd.h>

#define COLOR_RESET   "\033[0m"
#define COLOR_BLUE    "\033[1;34m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_CYAN    "\033[1;36m"

typedef struct {
    char *name;
    struct stat st;
    char path[1024];
} FileInfo;

int compare_files(const void *a, const void *b) {
    return strcmp(((FileInfo *)a)->name, ((FileInfo *)b)->name);
}

const char* get_color(struct stat *st) {
    if (S_ISLNK(st->st_mode)) return COLOR_CYAN;
    if (S_ISDIR(st->st_mode)) return COLOR_BLUE;
    if (st->st_mode & S_IXUSR) return COLOR_GREEN;
    return "";
}

void print_permissions(mode_t mode) {
    printf("%c", S_ISDIR(mode) ? 'd' : (S_ISLNK(mode) ? 'l' : '-'));
    printf("%c", (mode & S_IRUSR) ? 'r' : '-');
    printf("%c", (mode & S_IWUSR) ? 'w' : '-');
    printf("%c", (mode & S_IXUSR) ? 'x' : '-');
    printf("%c", (mode & S_IRGRP) ? 'r' : '-');
    printf("%c", (mode & S_IWGRP) ? 'w' : '-');
    printf("%c", (mode & S_IXGRP) ? 'x' : '-');
    printf("%c", (mode & S_IROTH) ? 'r' : '-');
    printf("%c", (mode & S_IWOTH) ? 'w' : '-');
    printf("%c", (mode & S_IXOTH) ? 'x' : '-');
}

void print_long_format(FileInfo *file) {
    struct stat *st = &file->st;
    
    print_permissions(st->st_mode);
    printf(" %2lu", (unsigned long)st->st_nlink);
    
    struct passwd *pw = getpwuid(st->st_uid);
    printf(" %-8s", pw ? pw->pw_name : "?");
    
    struct group *gr = getgrgid(st->st_gid);
    printf(" %-8s", gr ? gr->gr_name : "?");
    
    printf(" %8lld", (long long)st->st_size);
    
    char time_str[80];
    strftime(time_str, sizeof(time_str), "%b %d %H:%M", localtime(&st->st_mtime));
    printf(" %s", time_str);
    
    const char *color = get_color(st);
    printf(" %s%s%s", color, file->name, color[0] ? COLOR_RESET : "");
    
    if (S_ISLNK(st->st_mode)) {
        char link[1024];
        ssize_t len = readlink(file->path, link, sizeof(link) - 1);
        if (len != -1) {
            link[len] = '\0';
            printf(" -> %s", link);
        }
    }
    printf("\n");
}

void print_short_format(FileInfo *files, int count) {
    for (int i = 0; i < count; i++) {
        const char *color = get_color(&files[i].st);
        printf("%s%s%s", color, files[i].name, color[0] ? COLOR_RESET : "");
        if (i < count - 1) printf("  ");
    }
    if (count > 0) printf("\n");
}

void list_directory(const char *path, int show_hidden, int long_format) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror(path);
        return;
    }
    
    int capacity = 64, count = 0;
    FileInfo *files = malloc(capacity * sizeof(FileInfo));
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!show_hidden && entry->d_name[0] == '.') continue;
        
        if (count >= capacity) {
            capacity *= 2;
            files = realloc(files, capacity * sizeof(FileInfo));
        }
        
        files[count].name = strdup(entry->d_name);
        snprintf(files[count].path, sizeof(files[count].path), "%s/%s", path, entry->d_name);
        
        if (lstat(files[count].path, &files[count].st) == -1) {
            perror(files[count].path);
            free(files[count].name);
            continue;
        }
        count++;
    }
    closedir(dir);
    
    qsort(files, count, sizeof(FileInfo), compare_files);
    
    if (long_format) {
        long long total = 0;
        for (int i = 0; i < count; i++) {
            total += files[i].st.st_blocks;
        }
        printf("total %lld\n", total / 2);
        
        for (int i = 0; i < count; i++) {
            print_long_format(&files[i]);
        }
    } else {
        print_short_format(files, count);
    }
    
    for (int i = 0; i < count; i++) {
        free(files[i].name);
    }
    free(files);
}

int main(int argc, char *argv[]) {
    int show_hidden = 0, long_format = 0;
    
    int opt;
    while ((opt = getopt(argc, argv, "la")) != -1) {
        switch (opt) {
            case 'l': long_format = 1; break;
            case 'a': show_hidden = 1; break;
            default:
                fprintf(stderr, "Usage: %s [-l] [-a] [directory...]\n", argv[0]);
                return 1;
        }
    }
    
    if (optind >= argc) {
        list_directory(".", show_hidden, long_format);
    } else {
        for (int i = optind; i < argc; i++) {
            if (argc - optind > 1) {
                if (i > optind) printf("\n");
                printf("%s:\n", argv[i]);
            }
            list_directory(argv[i], show_hidden, long_format);
        }
    }
    
    return 0;
}
