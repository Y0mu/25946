#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ulimit.h>
#include <sys/resource.h>
#include <string.h>

#define MAXOPTS 100

extern char **environ;

void do_option(int c, char *arg)
{
    char buf[1024];
    long value;
    char *end;
    struct rlimit rl;
    char **env;

    switch (c) {
    case 'i':
        printf("uid=%d euid=%d gid=%d egid=%d\n",
               (int)getuid(), (int)geteuid(),
               (int)getgid(), (int)getegid());
        break;

    case 's':
        if (setpgid(0, 0) == -1)
            perror("setpgid");
        break;

    case 'p':
        printf("pid=%d ppid=%d pgid=%d\n",
               (int)getpid(),
               (int)getppid(),
               (int)getpgrp());
        break;

    case 'u':
        printf("ulimit=%ld\n", ulimit(UL_GETFSIZE));
        break;

    case 'U':
        value = strtol(arg, &end, 10);

        if (*end != '\0' || value < 0) {
            printf("bad value for -U: %s\n", arg);
            break;
        }

        if (ulimit(UL_SETFSIZE, value) == -1)
            perror("ulimit");

        break;

    case 'c':
        if (getrlimit(RLIMIT_CORE, &rl) == -1)
            perror("getrlimit");
        else if (rl.rlim_cur == RLIM_INFINITY)
            printf("core limit=unlimited\n");
        else
            printf("core limit=%ld\n",
                   (long)rl.rlim_cur);
        break;

    case 'C':
        value = strtol(arg, &end, 10);

        if (*end != '\0' || value < 0) {
            printf("bad value for -C: %s\n", arg);
            break;
        }

        if (getrlimit(RLIMIT_CORE, &rl) == -1) {
            perror("getrlimit");
            break;
        }

        rl.rlim_cur = value;

        if (setrlimit(RLIMIT_CORE, &rl) == -1)
            perror("setrlimit");

        break;

    case 'd':
        if (getcwd(buf, sizeof(buf)) != NULL)
            printf("cwd=%s\n", buf);
        else
            perror("getcwd");
        break;

    case 'v':
        for (env = environ; *env != NULL; env++)
            printf("%s\n", *env);
        break;

    case 'V':
        if (strchr(arg, '=') == NULL) {
            printf("bad value for -V: %s\n", arg);
            break;
        }

        if (putenv(arg) != 0)
            perror("putenv");

        break;
    }
}

int main(int argc, char *argv[])
{
    int c;
    int n = 0;
    int i;

    int opts[MAXOPTS];
    char *args[MAXOPTS];

    while ((c = getopt(argc, argv,
                       "ispuU:cC:dvV:")) != -1) {

        if (c == '?') {
            printf("unknown option\n");
            continue;
        }

        if (n >= MAXOPTS) {
            printf("too many options\n");
            return 1;
        }

        opts[n] = c;
        args[n] = optarg;
        n++;
    }

    for (i = n - 1; i >= 0; i--)
        do_option(opts[i], args[i]);

    return 0;
}
