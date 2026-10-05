#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <limits.h>
#include <string.h>
#include <errno.h>

extern char **environ;


/* Одна сохраненная опция */
typedef struct {
    int option;
    char *argument;
} Operation;


/* Проверка числового значения */
int parse_number(char *str, rlim_t *result)
{
    char *end;
    long value;

    if (str == NULL || *str == '\0') {
        return -1;
    }

    errno = 0;

    value = strtol(str, &end, 10);

    if (errno != 0 || *end != '\0' || value < 0) {
        return -1;
    }

    *result = (rlim_t)value;

    return 0;
}


/* -i */
void print_ids(void)
{
    printf("Real UID: %ld\n", (long)getuid());
    printf("Effective UID: %ld\n", (long)geteuid());
    printf("Real GID: %ld\n", (long)getgid());
    printf("Effective GID: %ld\n", (long)getegid());
}


/* -s */
void become_group_leader(void)
{
    if (setpgid(0, 0) == -1) {
        perror("setpgid");
    }
    else {
        printf("Process became group leader\n");
    }
}


/* -p */
void print_process_ids(void)
{
    printf("PID: %ld\n", (long)getpid());
    printf("PPID: %ld\n", (long)getppid());
    printf("PGID: %ld\n", (long)getpgrp());
}


/* -u */
void print_ulimit(void)
{
    struct rlimit limit;

    if (getrlimit(RLIMIT_NOFILE, &limit) == -1) {
        perror("getrlimit");
        return;
    }

    printf("ulimit: ");

    if (limit.rlim_cur == RLIM_INFINITY) {
        printf("unlimited\n");
    }
    else {
        printf("%lu\n", (unsigned long)limit.rlim_cur);
    }
}


/* -Unew_ulimit */
void change_ulimit(char *value)
{
    struct rlimit limit;
    rlim_t new_limit;

    if (parse_number(value, &new_limit) == -1) {
        fprintf(stderr, "Invalid value for -U: %s\n",
                value != NULL ? value : "(null)");
        return;
    }

    if (getrlimit(RLIMIT_NOFILE, &limit) == -1) {
        perror("getrlimit");
        return;
    }

    if (limit.rlim_max != RLIM_INFINITY &&
        new_limit > limit.rlim_max) {

        fprintf(stderr,
                "Cannot set ulimit above hard limit\n");
        return;
    }

    limit.rlim_cur = new_limit;

    if (setrlimit(RLIMIT_NOFILE, &limit) == -1) {
        perror("setrlimit");
        return;
    }

    printf("ulimit changed to %lu\n",
           (unsigned long)new_limit);
}


/* -c */
void print_core_limit(void)
{
    struct rlimit limit;

    if (getrlimit(RLIMIT_CORE, &limit) == -1) {
        perror("getrlimit");
        return;
    }

    printf("Core file size: ");

    if (limit.rlim_cur == RLIM_INFINITY) {
        printf("unlimited\n");
    }
    else {
        printf("%lu bytes\n",
               (unsigned long)limit.rlim_cur);
    }
}


/* -Csize */
void change_core_limit(char *value)
{
    struct rlimit limit;
    rlim_t new_limit;

    if (parse_number(value, &new_limit) == -1) {
        fprintf(stderr, "Invalid value for -C: %s\n",
                value != NULL ? value : "(null)");
        return;
    }

    if (getrlimit(RLIMIT_CORE, &limit) == -1) {
        perror("getrlimit");
        return;
    }

    if (limit.rlim_max != RLIM_INFINITY &&
        new_limit > limit.rlim_max) {

        fprintf(stderr,
                "Cannot set core size above hard limit\n");
        return;
    }

    limit.rlim_cur = new_limit;

    if (setrlimit(RLIMIT_CORE, &limit) == -1) {
        perror("setrlimit");
        return;
    }

    printf("Core file size changed to %lu bytes\n",
           (unsigned long)new_limit);
}


/* -d */
void print_directory(void)
{
    char directory[PATH_MAX];

    if (getcwd(directory, sizeof(directory)) == NULL) {
        perror("getcwd");
        return;
    }

    printf("Current directory: %s\n", directory);
}


/* -v */
void print_environment(void)
{
    char **env;

    env = environ;

    while (*env != NULL) {
        printf("%s\n", *env);
        env++;
    }
}


/* -Vname=value */
void change_environment(char *value)
{
    char *equal_sign;

    if (value == NULL) {
        fprintf(stderr, "Invalid value for -V\n");
        return;
    }

    equal_sign = strchr(value, '=');

    if (equal_sign == NULL || equal_sign == value) {
        fprintf(stderr,
                "Invalid value for -V: %s\n",
                value);
        fprintf(stderr,
                "Expected NAME=value\n");
        return;
    }

    if (putenv(value) != 0) {
        perror("putenv");
        return;
    }

    printf("Environment variable changed: %s\n",
           value);
}


int main(int argc, char *argv[])
{
    char *options;
    Operation *operations;
    Operation *tmp;

    int c;
    int count;

    options = "ispuU:cC:dvV:";

    operations = NULL;
    count = 0;

    /*
     * Не даем getopt самому печатать сообщения
     * об ошибках.
     */
    opterr = 0;


    /*
     * Сначала getopt читает все опции,
     * а мы сохраняем их в массив.
     */
    while ((c = getopt(argc, argv, options)) != -1) {

        if (c == '?') {

            if (optopt != 0) {
                fprintf(stderr,
                        "Invalid option or missing argument: -%c\n",
                        optopt);
            }
            else {
                fprintf(stderr,
                        "Invalid option\n");
            }

            free(operations);
            return 1;
        }


        tmp = realloc(operations,
                      (count + 1) * sizeof(Operation));

        if (tmp == NULL) {
            perror("realloc");
            free(operations);
            return 1;
        }

        operations = tmp;

        operations[count].option = c;
        operations[count].argument = optarg;

        count++;
    }


    /*
     * Выполняем опции СПРАВА НАЛЕВО.
     *
     * Например:
     *
     * ./options -i -p -s
     *
     * сначала -s,
     * потом -p,
     * потом -i.
     */
    while (count > 0) {

        count--;

        switch (operations[count].option) {

            case 'i':
                print_ids();
                break;


            case 's':
                become_group_leader();
                break;


            case 'p':
                print_process_ids();
                break;


            case 'u':
                print_ulimit();
                break;


            case 'U':
                change_ulimit(
                    operations[count].argument
                );
                break;


            case 'c':
                print_core_limit();
                break;


            case 'C':
                change_core_limit(
                    operations[count].argument
                );
                break;


            case 'd':
                print_directory();
                break;


            case 'v':
                print_environment();
                break;


            case 'V':
                change_environment(
                    operations[count].argument
                );
                break;


            default:
                fprintf(stderr,
                        "Unknown option\n");
                break;
        }
    }


    free(operations);

    return 0;
}
