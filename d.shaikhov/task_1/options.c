#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/types.h>

extern char **environ;

typedef struct {
    int flag;
    char *argument;
} Option;

int main(int argc, char *argv[]) {
    char *options = ":ispuU:cC:dvV:";
    int c;

    Option flags[100];
    int count = 0;
    int status = 0;

    opterr = 0;

    while ((c = getopt(argc, argv, options)) != -1) {

        if (c == '?') {
            printf("Invalid option: -%c\n", optopt);
            return 1;
        }

        if (c == ':') {
            printf("Missing argument for -%c\n", optopt);
            return 1;
        }

        if (count == 100) {
            printf("Too many options\n");
            return 1;
        }

        flags[count].flag = c;
        flags[count].argument = optarg;
        count++;
    }

    for (int i = count - 1; i >= 0; i--) {

        c = flags[i].flag;

        switch (c) {

            case 'i':
                printf("UID: %d\n", (int)getuid());
                printf("Effective UID: %d\n", (int)geteuid());
                printf("GID: %d\n", (int)getgid());
                printf("Effective GID: %d\n", (int)getegid());
                break;

            case 's':
                if (setpgid(0, 0) == -1) {
                    printf("setpgid: %s\n", strerror(errno));
                    status = 1;
                }
                break;

            case 'p':
                printf("PID: %d\n", (int)getpid());
                printf("PPID: %d\n", (int)getppid());
                printf("PGID: %d\n", (int)getpgrp());
                break;

            case 'u': {
                struct rlimit limit;

                if (getrlimit(RLIMIT_NOFILE, &limit) == -1) {
                    printf("getrlimit: %s\n", strerror(errno));
                    status = 1;
                    break;
                }

                if (limit.rlim_cur == RLIM_INFINITY) {
                    printf("ulimit: unlimited\n");
                } else {
                    printf("ulimit: %llu\n",
                           (unsigned long long)limit.rlim_cur);
                }
                break;
            }

            case 'U': {
                struct rlimit limit;
                char *end;
                long value;

                errno = 0;
                value = strtol(flags[i].argument, &end, 10);

                if (errno != 0 || end == flags[i].argument || *end != '\0' || value < 0) {
                    printf("Invalid value for -U\n");
                    status = 1;
                    break;
                }

                if (getrlimit(RLIMIT_NOFILE, &limit) == -1) {
                    printf("getrlimit: %s\n", strerror(errno));
                    status = 1;
                    break;
                }

                if ((unsigned long long)(rlim_t)value != (unsigned long long)value) {
                    printf("Value is too large\n");
                    status = 1;
                    break;
                }

                limit.rlim_cur = (rlim_t)value;

                if (setrlimit(RLIMIT_NOFILE, &limit) == -1) {
                    printf("setrlimit: %s\n", strerror(errno));
                    status = 1;
                }
                break;
            }

            case 'c': {
                struct rlimit limit;

                if (getrlimit(RLIMIT_CORE, &limit) == -1) {
                    printf("getrlimit: %s\n", strerror(errno));
                    status = 1;
                    break;
                }

                if (limit.rlim_cur == RLIM_INFINITY) {
                    printf("Core file size: unlimited\n");
                } else {
                    printf("Core file size: %llu bytes\n",
                           (unsigned long long)limit.rlim_cur);
                }
                break;
            }

            case 'C': {
                struct rlimit limit;
                char *end;
                long value;

                errno = 0;
                value = strtol(flags[i].argument, &end, 10);

                if (errno != 0 || end == flags[i].argument || *end != '\0' || value < 0) {
                    printf("Invalid value for -C\n");
                    status = 1;
                    break;
                }

                if (getrlimit(RLIMIT_CORE, &limit) == -1) {
                    printf("getrlimit: %s\n", strerror(errno));
                    status = 1;
                    break;
                }

                if ((unsigned long long)(rlim_t)value !=
                    (unsigned long long)value) {
                    printf("Value is too large\n");
                    status = 1;
                    break;
                }

                limit.rlim_cur = (rlim_t)value;

                if (setrlimit(RLIMIT_CORE, &limit) == -1) {
                    printf("setrlimit: %s\n", strerror(errno));
                    status = 1;
                }
                break;
            }

            case 'd': {
                char path[4096];

                if (getcwd(path, sizeof(path)) == NULL) {
                    printf("getcwd: %s\n", strerror(errno));
                    status = 1;
                } else {
                    printf("Directory: %s\n", path);
                }
                break;
            }

            case 'v':
                for (int j = 0; environ[j] != NULL; j++) {
                    printf("%s\n", environ[j]);
                }
                break;

            case 'V':
                if (strchr(flags[i].argument, '=') == NULL || flags[i].argument[0] == '=') {
                    printf("Use NAME=VALUE\n");
                    status = 1;
                } else if (putenv(flags[i].argument) != 0) {
                    printf("putenv: %s\n", strerror(errno));
                    status = 1;
                }
                break;
        }
    }

    return status;
}

