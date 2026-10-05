#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>

extern char **environ;

int main(int argc, char *argv[]) {
// All options: s, p, u, d don't need arguments
    char *options = "ispuU:cC:dvV:";
    int c;
    
    for (int i = 1, j = argc - 1; i < j; i++, j--) {
        char *tmp = argv[i];
        argv[i] = argv[j];
        argv[j] = tmp;
    }
    // Parse options
    while ((c = getopt(argc, argv, options)) != -1) {
        switch (c) {
            case 'i':
                printf("Real UID: %d, Effective UID: %d\n", getuid(), geteuid());
                printf("Real GID: %d, Effective GID: %d\n", getgid(), getegid());
                break;
            case 's':
                if (setpgid(0, 0) == -1) {
                        perror("setpgid failed");
                }
                break;
            case 'p':
                printf("PID: %d, PPID: %d, Process Group ID: %d\n", getpid(), getppid(), getpgrp());
                break;
            case 'u':
                {
                    struct rlimit rl;
                    if (getrlimit(RLIMIT_FSIZE, &rl) == -1) {
                        perror("getrlimit FSIZE");
                    } else {
                        printf("ulimit (FSIZE): %lld bytes\n", (long long)rl.rlim_cur);
                    }
                }
                break;
            case 'U':
                {
                    long new_lim = strtol(optarg, NULL, 10);

		if (new_lim < 0) {
            	    fprintf(stderr, "Error: limit value cannot be negative\n");
           	     break;
       		}
                    struct rlimit rl;
                    if (getrlimit(RLIMIT_FSIZE, &rl) == -1) {
                        perror("getrlimit FSIZE");
                    } else {
                        rl.rlim_cur = new_lim;
                        if (setrlimit(RLIMIT_FSIZE, &rl) == -1) {
                            perror("setrlimit FSIZE");
                        }
                    }
                }
                break;
            case 'c':
                {
                    struct rlimit rl;
                    if (getrlimit(RLIMIT_CORE, &rl) == -1) {
                        perror("getrlimit CORE");
                    } else {
                        printf("Core file limit: %lld bytes\n", (long long)rl.rlim_cur);
                    }
                }
                break;
            case 'C':
                {
                    long new_size = strtol(optarg, NULL, 10);

 		if (new_size < 0) {
                    fprintf(stderr, "Error: core file size cannot be negative\n");
                    break;
                }
                    struct rlimit rl;
                    if (getrlimit(RLIMIT_CORE, &rl) == -1) {
                        perror("getrlimit CORE");
                    } else {
                        rl.rlim_cur = new_size;
                        if (setrlimit(RLIMIT_CORE, &rl) == -1) {
                            perror("setrlimit CORE");
                        }
                    }
                }
                break;
            case 'd':
                {
                    char cwd[1024];
                    if (getcwd(cwd, sizeof(cwd)) != NULL) {
                        printf("Current Working Directory: %s\n", cwd);
                    } else {
                        perror("getcwd");
                    }
                }
                break;
            case 'v':
                {
                    for (int i = 0; environ[i] != NULL; i++) {
                        printf("%s\n", environ[i]);
                    }
                }
                break;
            case 'V':
                {
                    char *eq_sign = strchr(optarg, '=');
                    if (eq_sign != NULL) {
                        *eq_sign = '\0';
                        char *name = optarg;
                        char *value = eq_sign + 1;
                        
                        if (setenv(name, value, 1) == -1) {
                            perror("setenv");
                        }
                        *eq_sign = '=';
                    } else {
                        fprintf(stderr, "Error: For the -V option, use the format name=value\n");
                    }
                }
                break;
            case '?':
                printf("Invalid option: %c\n", optopt);
                break;
        }
    }
    
    return 0;
}
