#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

typedef struct {
    long offset;
    int length;
} LineInfo;

int main(int argc, char *argv[]) {
    int fd;
    char ch;

    LineInfo *lines = NULL;
    int num_lines = 0;
    int capacity = 0;

    long line_start = 0;
    int current_length = 0;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return 1;
    }

    fd = open(argv[1], O_RDONLY);

    if (fd == -1) {
        perror("open");
        return 1;
    }

    while (read(fd, &ch, 1) == 1) {
        current_length++;

        if (ch == '\n') {
            if (num_lines == capacity) {
                capacity = capacity == 0 ? 10 : capacity * 2;

                LineInfo *new_lines =
                    realloc(lines, capacity * sizeof(LineInfo));

                if (new_lines == NULL) {
                    perror("realloc");
                    free(lines);
                    close(fd);
                    return 1;
                }

                lines = new_lines;
            }

            lines[num_lines].offset = line_start;
            lines[num_lines].length = current_length;
            num_lines++;

            line_start = lseek(fd, 0, SEEK_CUR);

            if (line_start == -1) {
                perror("lseek");
                free(lines);
                close(fd);
                return 1;
            }

            current_length = 0;
        }
    }

    if (current_length > 0) {
        if (num_lines == capacity) {
            capacity = capacity == 0 ? 10 : capacity * 2;

            LineInfo *new_lines =
                realloc(lines, capacity * sizeof(LineInfo));

            if (new_lines == NULL) {
                perror("realloc");
                free(lines);
                close(fd);
                return 1;
            }

            lines = new_lines;
        }

        lines[num_lines].offset = line_start;
        lines[num_lines].length = current_length;
        num_lines++;
    }

    printf("--- Debug: Line Table ---\n");

    for (int i = 0; i < num_lines; i++) {
        printf("Line %d: Offset = %ld, Length = %d\n",
               i + 1,
               lines[i].offset,
               lines[i].length);
    }

    printf("-------------------------\n");

    while (1) {
        int line_number;

        printf("Enter line number (0 to quit): ");

        if (scanf("%d", &line_number) != 1) {
            break;
        }

        if (line_number == 0) {
            break;
        }

        if (line_number < 1 || line_number > num_lines) {
            printf("Invalid line number\n");
            continue;
        }

        LineInfo info = lines[line_number - 1];

        if (lseek(fd, info.offset, SEEK_SET) == -1) {
            perror("lseek");
            break;
        }

        char *buffer = malloc(info.length + 1);

        if (buffer == NULL) {
            perror("malloc");
            break;
        }

        ssize_t bytes_read = read(fd, buffer, info.length);

        if (bytes_read == -1) {
            perror("read");
            free(buffer);
            break;
        }

        buffer[bytes_read] = '\0';

        printf("%s", buffer);

        if (bytes_read > 0 && buffer[bytes_read - 1] != '\n') {
            printf("\n");
        }

        free(buffer);
    }

    free(lines);
    close(fd);

    return 0;
}
