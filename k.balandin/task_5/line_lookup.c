#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

typedef struct {
    long offset;
    int length;
} LineInfo;

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    LineInfo *table = NULL;
    int num_lines = 0;
    int capacity = 0;

    char ch;
    long current_pos = 0;
    long line_start = 0;
    int line_length = 0;

    while (read(fd, &ch, 1) == 1) {
        current_pos++;
        if (ch == '\n') {
            if (num_lines == capacity) {
                capacity = capacity == 0 ? 16 : capacity * 2;
                table = realloc(table, capacity * sizeof(LineInfo));
                if (table == NULL) {
                    fprintf(stderr, "Error: realloc failed\n");
                    close(fd);
                    return 1;
                }
            }
            table[num_lines].offset = line_start;
            table[num_lines].length = line_length;
            num_lines++;
            line_start = current_pos;
            line_length = 0;
        } else {
            line_length++;
        }
    }

    if (line_length > 0) {
        if (num_lines == capacity) {
            capacity = capacity == 0 ? 16 : capacity * 2;
            table = realloc(table, capacity * sizeof(LineInfo));
            if (table == NULL) {
                fprintf(stderr, "Error: realloc failed\n");
                close(fd);
                return 1;
            }
        }
        table[num_lines].offset = line_start;
        table[num_lines].length = line_length;
        num_lines++;
    }

    printf("--- Debug: Line Table ---\n");
    for (int i = 0; i < num_lines; i++) {
        printf("Line %d: Offset = %ld, Length = %d\n",
               i + 1, table[i].offset, table[i].length);
    }
    printf("-------------------------\n");

    int line_num;
    while (1) {
        printf("Enter line number (0 to quit): ");
        fflush(stdout);

        if (scanf("%d", &line_num) != 1) {
            break;
        }

        if (line_num == 0) {
            break;
        }

        if (line_num < 1 || line_num > num_lines) {
            printf("Invalid line number. Valid range: 1..%d\n", num_lines);
            continue;
        }

        long offset = table[line_num - 1].offset;
        int length = table[line_num - 1].length;

        if (lseek(fd, offset, SEEK_SET) == -1) {
            perror("lseek");
            continue;
        }

        char *buffer = malloc(length + 1);
        if (buffer == NULL) {
            fprintf(stderr, "Error: malloc failed\n");
            continue;
        }

        ssize_t bytes_read = read(fd, buffer, length);
        if (bytes_read == -1) {
            perror("read");
            free(buffer);
            continue;
        }

        buffer[bytes_read] = '\0';
        printf("%s\n", buffer);
        free(buffer);
    }

    free(table);
    close(fd);
    return 0;
}
