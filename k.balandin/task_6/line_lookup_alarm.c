#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>

typedef struct {
    long offset;
    int length;
} LineInfo;

static int g_fd = -1;
static LineInfo *g_table = NULL;
static int g_num_lines = 0;

void alarm_handler(int sig)
{
    (void)sig;
    const char msg[] = "\n[TIMEOUT] Time is up! Printing full file content...\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);

    if (lseek(g_fd, 0, SEEK_SET) == -1) {
        _exit(EXIT_FAILURE);
    }

    char buf[1024];
    ssize_t n;
    while ((n = read(g_fd, buf, sizeof(buf))) > 0) {
        ssize_t written = 0;
        while (written < n) {
            ssize_t w = write(STDOUT_FILENO, buf + written, n - written);
            if (w <= 0) {
                _exit(EXIT_FAILURE);
            }
            written += w;
        }
    }

    _exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    g_fd = open(argv[1], O_RDONLY);
    if (g_fd == -1) {
        perror("open");
        return 1;
    }

    int capacity = 0;
    char ch;
    long current_pos = 0;
    long line_start = 0;
    int line_length = 0;

    while (read(g_fd, &ch, 1) == 1) {
        current_pos++;
        if (ch == '\n') {
            if (g_num_lines == capacity) {
                capacity = capacity == 0 ? 16 : capacity * 2;
                LineInfo *tmp = realloc(g_table, capacity * sizeof(LineInfo));
                if (tmp == NULL) {
                    fprintf(stderr, "Error: realloc failed\n");
                    close(g_fd);
                    free(g_table);
                    return 1;
                }
                g_table = tmp;
            }
            g_table[g_num_lines].offset = line_start;
            g_table[g_num_lines].length = line_length;
            g_num_lines++;
            line_start = current_pos;
            line_length = 0;
        } else {
            line_length++;
        }
    }

    if (line_length > 0) {
        if (g_num_lines == capacity) {
            capacity = capacity == 0 ? 16 : capacity * 2;
            LineInfo *tmp = realloc(g_table, capacity * sizeof(LineInfo));
            if (tmp == NULL) {
                fprintf(stderr, "Error: realloc failed\n");
                close(g_fd);
                free(g_table);
                return 1;
            }
            g_table = tmp;
        }
        g_table[g_num_lines].offset = line_start;
        g_table[g_num_lines].length = line_length;
        g_num_lines++;
    }

    printf("--- Debug: Line Table ---\n");
    for (int i = 0; i < g_num_lines; i++) {
        printf("Line %d: Offset = %ld, Length = %d\n",
               i + 1, g_table[i].offset, g_table[i].length);
    }
    printf("-------------------------\n");

    signal(SIGALRM, alarm_handler);

    int line_num;
    while (1) {
        printf("Enter line number (0 to quit, 5 sec timeout): ");
        fflush(stdout);

        alarm(5);

        int result = scanf("%d", &line_num);

        alarm(0);

        if (result != 1) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {
                ;
            }
            if (c == EOF) {
                break;
            }
            printf("Invalid input. Try again.\n");
            continue;
        }

        if (line_num == 0) {
            break;
        }

        if (line_num < 1 || line_num > g_num_lines) {
            printf("Invalid line number. Valid range: 1..%d\n", g_num_lines);
            continue;
        }

        long offset = g_table[line_num - 1].offset;
        int length = g_table[line_num - 1].length;

        if (lseek(g_fd, offset, SEEK_SET) == -1) {
            perror("lseek");
            continue;
        }

        char *buffer = malloc(length + 1);
        if (buffer == NULL) {
            fprintf(stderr, "Error: malloc failed\n");
            continue;
        }

        ssize_t bytes_read = read(g_fd, buffer, length);
        if (bytes_read == -1) {
            perror("read");
            free(buffer);
            continue;
        }

        buffer[bytes_read] = '\0';
        printf("%s\n", buffer);
        free(buffer);
    }

    free(g_table);
    close(g_fd);
    return 0;
}
