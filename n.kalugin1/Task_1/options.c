#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <limits.h>
#include <string.h>
#include <errno.h>

// Программа рассчитана на Solaris/Linux: Windows не предоставляет эти POSIX API.
extern char **environ;

struct Option
{
    int name;
    char *argument;
};

void print_limit(rlim_t value, int is_byte_count)
{
    if (value == RLIM_INFINITY)
        printf("unlimited\n");
    else if (is_byte_count)
        printf("%llu bytes\n", (unsigned long long)value);
    else
        printf("%llu\n", (unsigned long long)value);
}

int parse_limit(const char *text, rlim_t *value)
{
    char *end;
    unsigned long long number;

    if (text == NULL || text[0] == '\0' || text[0] == '-')
        return 0;

    errno = 0;
    number = strtoull(text, &end, 10);
    if (errno == ERANGE || *end != '\0')
        return 0;

    *value = (rlim_t)number;
    if ((unsigned long long)*value != number)
        return 0;

    return 1;
}

void print_resource_limit(int resource, int is_byte_count)
{
    struct rlimit limit;

    if (getrlimit(resource, &limit) == -1)
    {
        perror("getrlimit");
        return;
    }

    print_limit(limit.rlim_cur, is_byte_count);
}

void change_resource_limit(int resource, const char *text)
{
    struct rlimit limit;
    rlim_t value;

    if (!parse_limit(text, &value))
    {
        fprintf(stderr, "Invalid limit value: %s\n", text);
        return;
    }

    if (getrlimit(resource, &limit) == -1)
    {
        perror("getrlimit");
        return;
    }

    // Меняем мягкий лимит, не трогая установленный системой жёсткий лимит.
    limit.rlim_cur = value;
    if (setrlimit(resource, &limit) == -1)
        perror("setrlimit");
}

void run_option(const struct Option *option)
{
    char directory[PATH_MAX];
    char *equal_sign;
    char *name;
    size_t name_length;
    size_t i;

    switch (option->name)
    {
        case 'i':
            printf("Real UID: %lu, Effective UID: %lu\n",
                   (unsigned long)getuid(), (unsigned long)geteuid());
            printf("Real GID: %lu, Effective GID: %lu\n",
                   (unsigned long)getgid(), (unsigned long)getegid());
            break;
        case 's':
            if (setpgid(0, 0) == -1)
                perror("setpgid");
            break;
        case 'p':
            printf("PID: %ld, PPID: %ld, PGID: %ld\n",
                   (long)getpid(), (long)getppid(), (long)getpgrp());
            break;
        case 'u':
            print_resource_limit(RLIMIT_NOFILE, 0);
            break;
        case 'U':
            change_resource_limit(RLIMIT_NOFILE, option->argument);
            break;
        case 'c':
            print_resource_limit(RLIMIT_CORE, 1);
            break;
        case 'C':
            change_resource_limit(RLIMIT_CORE, option->argument);
            break;
        case 'd':
            if (getcwd(directory, sizeof(directory)) == NULL)
                perror("getcwd");
            else
                printf("%s\n", directory);
            break;
        case 'v':
            for (i = 0; environ[i] != NULL; ++i)
                printf("%s\n", environ[i]);
            break;
        case 'V':
            // setenv принимает имя и значение отдельно, поэтому делим NAME=VALUE.
            equal_sign = strchr(option->argument, '=');
            if (equal_sign == NULL || equal_sign == option->argument)
            {
                fprintf(stderr, "Expected -Vname=value\n");
                break;
            }

            name_length = (size_t)(equal_sign - option->argument);
            name = (char *)malloc(name_length + 1);
            if (name == NULL)
            {
                perror("malloc");
                break;
            }

            memcpy(name, option->argument, name_length);
            name[name_length] = '\0';
            if (setenv(name, equal_sign + 1, 1) == -1)
                perror("setenv");
            free(name);
            break;
    }
}

int main(int argc, char *argv[])
{
    struct Option *options;
    size_t capacity = 0;
    int count = 0;
    int option_name;
    int i;

    for (i = 1; i < argc; ++i)
        capacity += strlen(argv[i]);

    if (capacity == 0)
        capacity = 1;

    options = (struct Option *)malloc(capacity * sizeof(struct Option));
    if (options == NULL)
    {
        perror("malloc");
        return 1;
    }

    opterr = 0;
    while ((option_name = getopt(argc, argv, "ispuU:cC:dvV:")) != -1)
    {
        if (option_name == '?')
        {
            fprintf(stderr, "Unknown or incomplete option: -%c\n", optopt);
            continue;
        }

        options[count].name = option_name;
        options[count].argument = optarg;
        ++count;
    }

    // getopt разбирает слева направо, а по условию опции выполняются справа налево.
    for (i = count - 1; i >= 0; --i)
        run_option(&options[i]);

    free(options);
    return 0;
}