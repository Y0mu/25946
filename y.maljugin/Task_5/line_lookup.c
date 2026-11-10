#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef TIMED_LOOKUP
#include <signal.h>
#endif

struct Line {
    off_t offset;
    size_t length;
};

#ifdef TIMED_LOOKUP
static int alarm_fd;

/* Only async-signal-safe system calls are used in the signal handler. */
static void on_alarm(int signal_number)
{
    (void)signal_number;
    static const char message[] = "\nTime is up; full file follows:\n";
    (void)write(STDOUT_FILENO, message, sizeof(message) - 1);
    if (lseek(alarm_fd, 0, SEEK_SET) == (off_t)-1)
        _exit(EXIT_FAILURE);
    char buffer[4096];
    ssize_t got;
    while ((got = read(alarm_fd, buffer, sizeof(buffer))) > 0) {
        size_t done = 0;
        while (done < (size_t)got) {
            ssize_t written = write(STDOUT_FILENO, buffer + done, (size_t)got - done);
            if (written <= 0)
                _exit(EXIT_FAILURE);
            done += (size_t)written;
        }
    }
    _exit(got == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
#endif

static int add_line(struct Line **lines, size_t *count, size_t *capacity,
                    off_t start, off_t end)
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
    (*lines)[*count].length = (size_t)(end - start);
    ++*count;
    return 0;
}

static int make_index(int fd, struct Line **lines, size_t *count)
{
    size_t capacity = 0;
    off_t start = 0;
    char ch;
    ssize_t got;

    while ((got = read(fd, &ch, 1)) == 1) {
        if (ch == '\n') {
            off_t end = lseek(fd, 0, SEEK_CUR);
            if (end == (off_t)-1 || add_line(lines, count, &capacity, start, end) == -1)
                return -1;
            start = end;
        }
    }
    if (got == -1)
        return -1;
    off_t end = lseek(fd, 0, SEEK_CUR);
    if (end == (off_t)-1)
        return -1;
    if (end > start && add_line(lines, count, &capacity, start, end) == -1)
        return -1;
    return 0;
}

static int print_line(int fd, struct Line line)
{
    if (lseek(fd, line.offset, SEEK_SET) == (off_t)-1)
        return -1;
    char *text = malloc(line.length + 1);
    if (text == NULL)
        return -1;
    size_t done = 0;
    while (done < line.length) {
        ssize_t got = read(fd, text + done, line.length - done);
        if (got <= 0) {
            free(text);
            return -1;
        }
        done += (size_t)got;
    }
    text[line.length] = '\0';
    printf("%s", text);
    if (line.length == 0 || text[line.length - 1] != '\n')
        putchar('\n');
    free(text);
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
    struct Line *lines = NULL;
    size_t count = 0;
    if (make_index(fd, &lines, &count) == -1) {
        perror("index");
        free(lines);
        close(fd);
        return EXIT_FAILURE;
    }
    for (size_t i = 0; i < count; ++i)
        printf("Line %lu: offset=%lld, length=%lu\n", (unsigned long)(i + 1),
               (long long)lines[i].offset, (unsigned long)lines[i].length);

#ifdef TIMED_LOOKUP
    alarm_fd = fd;
    struct sigaction action = {0};
    action.sa_handler = on_alarm;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGALRM, &action, NULL) == -1) {
        perror("sigaction");
        free(lines);
        close(fd);
        return EXIT_FAILURE;
    }
#endif

    char input[64];
    int result = EXIT_SUCCESS;
    for (;;) {
        printf("Line number (0 to quit"
#ifdef TIMED_LOOKUP
               ", 5 seconds"
#endif
               "): ");
        fflush(stdout);
#ifdef TIMED_LOOKUP
        alarm(5);
#endif
        char *answer = fgets(input, sizeof(input), stdin);
#ifdef TIMED_LOOKUP
        alarm(0);
#endif
        if (answer == NULL)
            break;
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
        if (print_line(fd, lines[number - 1]) == -1) {
            perror("read line");
            result = EXIT_FAILURE;
            break;
        }
    }
    free(lines);
    if (close(fd) == -1) {
        perror("close");
        result = EXIT_FAILURE;
    }
    return result;
}
