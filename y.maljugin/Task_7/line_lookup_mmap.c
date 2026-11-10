#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

struct Line {
    size_t offset;
    size_t length;
};

static volatile sig_atomic_t timed_out;

static void on_alarm(int signal_number)
{
    (void)signal_number;
    timed_out = 1;
}

static int add_line(struct Line **lines, size_t *count, size_t *capacity,
                    size_t start, size_t end)
{
    if (*count == *capacity) {
        size_t next = *capacity ? *capacity * 2 : 16;
        struct Line *grown = realloc(*lines, next * sizeof(**lines));
        if (grown == NULL)
            return -1;
        *lines = grown;
        *capacity = next;
    }
    (*lines)[*count].offset = start;
    (*lines)[*count].length = end - start;
    ++*count;
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s text-file\n", argv[0]);
        return EXIT_FAILURE;
    }
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }
    struct stat info;
    if (fstat(fd, &info) == -1 || info.st_size < 0) {
        perror("fstat");
        close(fd);
        return EXIT_FAILURE;
    }
    if (info.st_size == 0) {
        puts("File is empty.");
        close(fd);
        return EXIT_SUCCESS;
    }
    size_t size = (size_t)info.st_size;
    const char *map = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return EXIT_FAILURE;
    }
    close(fd);

    struct Line *lines = NULL;
    size_t count = 0;
    size_t capacity = 0;
    size_t start = 0;
    for (size_t i = 0; i < size; ++i) {
        if (map[i] == '\n') {
            if (add_line(&lines, &count, &capacity, start, i + 1) == -1)
                goto memory_error;
            start = i + 1;
        }
    }
    if (start < size && add_line(&lines, &count, &capacity, start, size) == -1)
        goto memory_error;

    for (size_t i = 0; i < count; ++i)
        printf("Line %lu: offset=%lu, length=%lu\n", (unsigned long)(i + 1),
               (unsigned long)lines[i].offset, (unsigned long)lines[i].length);

    struct sigaction action = {0};
    action.sa_handler = on_alarm;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGALRM, &action, NULL) == -1) {
        perror("sigaction");
        free(lines);
        munmap((void *)map, size);
        return EXIT_FAILURE;
    }

    char input[64];
    int result = EXIT_SUCCESS;
    for (;;) {
        printf("Line number (0 to quit, 5 seconds): ");
        fflush(stdout);
        timed_out = 0;
        alarm(5);
        char *answer = fgets(input, sizeof(input), stdin);
        alarm(0);
        if (timed_out) {
            puts("\nTime is up; full file follows:");
            if (fwrite(map, 1, size, stdout) != size)
                result = EXIT_FAILURE;
            break;
        }
        if (answer == NULL) {
            if (ferror(stdin)) {
                perror("input");
                result = EXIT_FAILURE;
            }
            break;
        }
        char *end;
        errno = 0;
        long number = strtol(input, &end, 10);
        if (errno != 0 || end == input || (*end != '\n' && *end != '\0') || number < 0) {
            fprintf(stderr, "Enter a non-negative line number.\n");
            continue;
        }
        if (number == 0)
            break;
        if ((unsigned long)number > count) {
            fprintf(stderr, "There are only %lu lines.\n", (unsigned long)count);
            continue;
        }
        struct Line line = lines[number - 1];
        if (fwrite(map + line.offset, 1, line.length, stdout) != line.length) {
            perror("output");
            result = EXIT_FAILURE;
            break;
        }
        if (line.length == 0 || map[line.offset + line.length - 1] != '\n')
            putchar('\n');
    }
    free(lines);
    if (munmap((void *)map, size) == -1) {
        perror("munmap");
        result = EXIT_FAILURE;
    }
    return result;

memory_error:
    perror("realloc");
    free(lines);
    munmap((void *)map, size);
    return EXIT_FAILURE;
}
