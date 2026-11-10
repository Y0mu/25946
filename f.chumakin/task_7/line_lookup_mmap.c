#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define MAXLINES 100

struct Line {
    long offset;
    int length;
};

char *file_map;
size_t file_size;

void alarm_handler(int sig)
{
    printf("\n[TIMEOUT] Time is up! Printing full file content...\n");
    write(STDOUT_FILENO, file_map, file_size);
    _exit(0);
}

int main(int argc, char *argv[])
{
    int fd;
    struct stat st;
    struct Line lines[MAXLINES];
    int n = 0;
    int len = 0;
    long start = 0;
    int num;
    size_t i;

    if (argc != 2) {
        printf("Usage: %s file\n", argv[0]);
        return 1;
    }

    fd = open(argv[1], O_RDONLY);

    if (fd == -1) {
        perror("open");
        return 1;
    }

    if (fstat(fd, &st) == -1) {
        perror("fstat");
        close(fd);
        return 1;
    }

    file_size = st.st_size;

    if (file_size == 0) {
        printf("File is empty.\n");
        close(fd);
        return 0;
    }

    file_map = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);

    if (file_map == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    close(fd);

    for (i = 0; i < file_size; i++) {
        if (file_map[i] == '\n') {
            lines[n].offset = start;
            lines[n].length = len;
            n++;

            start = i + 1;
            len = 0;
        } else {
            len++;
        }
    }

    if (len > 0) {
        lines[n].offset = start;
        lines[n].length = len;
        n++;
    }

    signal(SIGALRM, alarm_handler);

    while (1) {
        printf("Enter line number (0 to quit, 5 sec timeout): ");
        fflush(stdout);

        alarm(5);

        if (scanf("%d", &num) != 1) {
            alarm(0);
            break;
        }

        alarm(0);

        if (num == 0)
            break;

        if (num < 1 || num > n) {
            printf("Invalid line number\n");
            continue;
        }

        fwrite(file_map + lines[num - 1].offset,
               1,
               lines[num - 1].length,
               stdout);

        printf("\n");
    }

    munmap(file_map, file_size);

    return 0;
}
