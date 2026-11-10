#define _POSIX_C_SOURCE 200809L
#include "line_lookup.h"
#include <fcntl.h>
#include <sys/stat.h>

static int build_table(int fd, LineTable *table)
{
    char buffer[4096];
    off_t start = 0;
    size_t length = 0;
    for (;;) {
        ssize_t count = read(fd, buffer, sizeof(buffer));
        off_t end;
        if (count == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (count == 0) break;
        end = lseek(fd, 0, SEEK_CUR);
        if (end == (off_t)-1) return -1;
        for (ssize_t i = 0; i < count; ++i) {
            if (length == SIZE_MAX) {
                errno = EOVERFLOW;
                return -1;
            }
            ++length;
            if (buffer[i] == '\n') {
                /* Длина включает перевод строки, смещения измеряются в байтах */
                if (table_add(table, start, length) == -1) return -1;
                start = end - count + i + 1;
                length = 0;
            }
        }
    }
    return length != 0 ? table_add(table, start, length) : 0;
}

static int print_line(int fd, const LineInfo *line)
{
    char *buffer;
    size_t received = 0;
    if (line->length == SIZE_MAX) {
        errno = EOVERFLOW;
        return -1;
    }
    if (lseek(fd, line->offset, SEEK_SET) == (off_t)-1) return -1;
    buffer = malloc(line->length + 1);
    if (buffer == NULL) return -1;
    /* read может вернуть меньше байт, поэтому дочитываем строку в цикле */
    while (received < line->length) {
        size_t wanted = line->length - received;
        ssize_t count;
        if (wanted > 4096) wanted = 4096;
        count = read(fd, buffer + received, wanted);
        if (count == -1 && errno == EINTR) continue;
        if (count <= 0) {
            if (count == 0) errno = EIO;
            free(buffer);
            return -1;
        }
        received += (size_t)count;
    }
    buffer[received] = '\0';
    printf("%s", buffer);
    if (received != 0 && buffer[received - 1] != '\n') putchar('\n');
    free(buffer);
    return ferror(stdout) ? -1 : 0;
}

static int print_file(int fd)
{
    char buffer[4096];
    if (fflush(stdout) == EOF || lseek(fd, 0, SEEK_SET) == (off_t)-1) return -1;
    for (;;) {
        ssize_t count = read(fd, buffer, sizeof(buffer));
        size_t sent = 0;
        if (count == -1 && errno == EINTR) continue;
        if (count == -1) return -1;
        if (count == 0) return 0;
        while (sent < (size_t)count) {
            ssize_t written = write(STDOUT_FILENO, buffer + sent, (size_t)count - sent);
            if (written == -1 && errno == EINTR) continue;
            if (written <= 0) {
                if (written == 0) errno = EIO;
                return -1;
            }
            sent += (size_t)written;
        }
    }
}

int run_lookup_io(int argc, char **argv, unsigned int seconds)
{
    LineTable table = { NULL, 0, 0 };
    struct stat info;
    int fd, status = EXIT_SUCCESS;
    if (argc != 2) {
        fprintf(stderr, "Usage: %s FILE\n", argv[0]);
        return EXIT_FAILURE;
    }
    fd = open(argv[1], O_RDONLY);
    if (fd == -1) { perror("open"); return EXIT_FAILURE; }
    if (fstat(fd, &info) == -1) { perror("fstat"); status = EXIT_FAILURE; goto cleanup; }
    if (!S_ISREG(info.st_mode)) {
        fprintf(stderr, "Expected a regular text file\n");
        status = EXIT_FAILURE;
        goto cleanup;
    }
    if (build_table(fd, &table) == -1) { perror("build table"); status = EXIT_FAILURE; goto cleanup; }
    if (table.count == 0) { puts("File is empty."); goto cleanup; }
    table_print(&table);
    if (input_init() != 0) { perror("initialize input"); status = EXIT_FAILURE; goto cleanup; }
    for (;;) {
        size_t number;
        int result;
        printf("Enter line number (0 to quit%s): ", seconds ? ", 5 sec timeout" : "");
        fflush(stdout);
        result = read_number(seconds, &number);
        if (result == INPUT_TIMEOUT) {
            puts("\n[TIMEOUT] Printing full file content...");
            if (print_file(fd) == -1) { perror("print file"); status = EXIT_FAILURE; }
            break;
        }
        if (result == INPUT_END) break;
        if (result == INPUT_ERROR) { perror("read input"); status = EXIT_FAILURE; break; }
        if (result == INPUT_INVALID) { puts("Enter a non-negative integer"); continue; }
        if (number == 0) break;
        if (number > table.count) { printf("Line number must be between 1 and %zu\n", table.count); continue; }
        if (print_line(fd, &table.items[number - 1]) == -1) { perror("read line"); status = EXIT_FAILURE; break; }
    }
cleanup:
    free(table.items);
    if (close(fd) == -1) { perror("close"); status = EXIT_FAILURE; }
    return status;
}
