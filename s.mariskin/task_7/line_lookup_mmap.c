#define _POSIX_C_SOURCE 200809L
#include "../common/line_lookup.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

int main(int argc, char **argv)
{
    LineTable table = { NULL, 0, 0 };
    struct stat info;
    char *map = MAP_FAILED;
    size_t size = 0, start = 0;
    int fd, status = EXIT_SUCCESS;
    if (argc != 2) {
        fprintf(stderr, "Usage: %s FILE\n", argv[0]);
        return EXIT_FAILURE;
    }
    fd = open(argv[1], O_RDONLY);
    if (fd == -1) { perror("open"); return EXIT_FAILURE; }
    if (fstat(fd, &info) == -1) { perror("fstat"); status = EXIT_FAILURE; goto cleanup; }
    if (!S_ISREG(info.st_mode) || info.st_size < 0 || (uintmax_t)info.st_size > SIZE_MAX) {
        fprintf(stderr, "Expected a regular file that fits in memory\n");
        status = EXIT_FAILURE;
        goto cleanup;
    }
    if (info.st_size == 0) { puts("File is empty."); goto cleanup; }
    size = (size_t)info.st_size;
    /* Отображаем файл только для чтения, данные доступны как массив байтов */
    map = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) { perror("mmap"); status = EXIT_FAILURE; goto cleanup; }
    {
        int close_result = close(fd);
        fd = -1;
        if (close_result == -1) { perror("close"); status = EXIT_FAILURE; goto cleanup; }
    }
    for (size_t i = 0; i < size; ++i) {
        if (map[i] == '\n') {
            if (table_add(&table, (off_t)start, i - start + 1) == -1) {
                perror("grow table"); status = EXIT_FAILURE; goto cleanup;
            }
            start = i + 1;
        }
    }
    if (start < size && table_add(&table, (off_t)start, size - start) == -1) {
        perror("grow table"); status = EXIT_FAILURE; goto cleanup;
    }
    table_print(&table);
    if (input_init() != 0) { perror("initialize input"); status = EXIT_FAILURE; goto cleanup; }
    for (;;) {
        size_t number;
        int result;
        printf("Enter line number (0 to quit, 5 sec timeout): ");
        fflush(stdout);
        result = read_number(5, &number);
        if (result == INPUT_TIMEOUT) {
            puts("\n[TIMEOUT] Printing full file content...");
            if (fwrite(map, 1, size, stdout) != size) { perror("print file"); status = EXIT_FAILURE; }
            break;
        }
        if (result == INPUT_END) break;
        if (result == INPUT_ERROR) { perror("read input"); status = EXIT_FAILURE; break; }
        if (result == INPUT_INVALID) { puts("Enter a non-negative integer"); continue; }
        if (number == 0) break;
        if (number > table.count) { printf("Line number must be between 1 and %zu\n", table.count); continue; }
        {
            LineInfo *line = &table.items[number - 1];
            /* fwrite выводит точную длину строки, завершающий ноль в отображении не нужен */
            const char *text = map + (size_t)line->offset;
            if (fwrite(text, 1, line->length, stdout) != line->length) {
                perror("print line"); status = EXIT_FAILURE; break;
            }
            if (text[line->length - 1] != '\n') putchar('\n');
        }
    }
cleanup:
    free(table.items);
    if (map != MAP_FAILED && munmap(map, size) == -1) { perror("munmap"); status = EXIT_FAILURE; }
    if (fd != -1 && close(fd) == -1) { perror("close"); status = EXIT_FAILURE; }
    if (fflush(stdout) == EOF) { perror("flush stdout"); status = EXIT_FAILURE; }
    return status;
}
