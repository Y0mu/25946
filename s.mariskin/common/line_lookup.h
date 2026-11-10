#ifndef LINE_LOOKUP_H
#define LINE_LOOKUP_H

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct {
    off_t offset;
    size_t length;
} LineInfo;

typedef struct {
    LineInfo *items;
    size_t count;
    size_t capacity;
} LineTable;

static int table_add(LineTable *table, off_t offset, size_t length)
{
    if (table->count == table->capacity) {
        size_t capacity = table->capacity == 0 ? 16 : table->capacity * 2;
        LineInfo *items;
        if (capacity < table->capacity || capacity > SIZE_MAX / sizeof(*items)) {
            errno = EOVERFLOW;
            return -1;
        }
        items = realloc(table->items, capacity * sizeof(*items));
        if (items == NULL) {
            return -1;
        }
        table->items = items;
        table->capacity = capacity;
    }
    table->items[table->count].offset = offset;
    table->items[table->count].length = length;
    ++table->count;
    return 0;
}

static void table_print(const LineTable *table)
{
    puts("--- Line table ---");
    for (size_t i = 0; i < table->count; ++i) {
        printf("Line %zu: Offset = %jd, Length = %zu\n", i + 1,
               (intmax_t)table->items[i].offset, table->items[i].length);
    }
}

static volatile sig_atomic_t timed_out = 0;

static void alarm_handler(int signal_number)
{
    (void)signal_number;
    /* В обработчике меняем только флаг, вывод и очистку выполняет основной код */
    timed_out = 1;
}

static int input_init(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = alarm_handler;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGALRM, &action, NULL) == -1) {
        return -1;
    }
    /* Отключаем буферизацию, чтобы готовность stdin соответствовала следующему символу */
    return setvbuf(stdin, NULL, _IONBF, 0);
}

enum InputResult { INPUT_ERROR = -1, INPUT_END, INPUT_NUMBER, INPUT_INVALID, INPUT_TIMEOUT };

static int read_number(unsigned int seconds, size_t *number)
{
    char buffer[128];
    size_t used = 0;
    int too_long = 0;
    int result = INPUT_END;
    sigset_t alarm_set, old_mask, wait_mask;

    sigemptyset(&alarm_set);
    sigaddset(&alarm_set, SIGALRM);
    if (sigprocmask(SIG_BLOCK, &alarm_set, &old_mask) == -1) {
        return INPUT_ERROR;
    }
    wait_mask = old_mask;
    sigdelset(&wait_mask, SIGALRM);
    timed_out = 0;
    alarm(seconds);

    for (;;) {
        fd_set ready;
        int available, character;
        FD_ZERO(&ready);
        FD_SET(STDIN_FILENO, &ready);
        /* pselect атомарно разрешает SIGALRM на время ожидания, исключая гонку перед блокировкой */
        available = pselect(STDIN_FILENO + 1, &ready, NULL, NULL, NULL, &wait_mask);
        if (timed_out) {
            result = INPUT_TIMEOUT;
            break;
        }
        if (available == -1) {
            if (errno == EINTR) {
                continue;
            }
            result = INPUT_ERROR;
            break;
        }
        character = getchar();
        if (character == EOF) {
            if (ferror(stdin)) {
                result = INPUT_ERROR;
            } else if (used != 0 || too_long) {
                result = INPUT_NUMBER;
            }
            break;
        }
        if (character == '\n') {
            result = INPUT_NUMBER;
            break;
        }
        if (character == '\0' || used == sizeof(buffer) - 1) {
            too_long = 1;
        } else {
            buffer[used++] = (char)character;
        }
    }

    alarm(0);
    /* Убираем отложенный сигнал старого таймера перед следующим запросом */
    {
        sigset_t pending;
        if (sigpending(&pending) == -1) {
            result = INPUT_ERROR;
        } else if (sigismember(&pending, SIGALRM)) {
            int caught;
            if (sigwait(&alarm_set, &caught) != 0) {
                result = INPUT_ERROR;
            }
        }
    }
    if (sigprocmask(SIG_SETMASK, &old_mask, NULL) == -1) {
        return INPUT_ERROR;
    }
    if (result == INPUT_NUMBER) {
        char *start, *end;
        uintmax_t value;
        buffer[used] = '\0';
        start = buffer;
        while (isspace((unsigned char)*start)) {
            ++start;
        }
        if (too_long || !isdigit((unsigned char)*start)) {
            return INPUT_INVALID;
        }
        errno = 0;
        value = strtoumax(start, &end, 10);
        while (isspace((unsigned char)*end)) {
            ++end;
        }
        if (errno == ERANGE || *end != '\0' || value > SIZE_MAX) {
            return INPUT_INVALID;
        }
        *number = (size_t)value;
    }
    return result;
}

int run_lookup_io(int argc, char **argv, unsigned int seconds);

#endif
