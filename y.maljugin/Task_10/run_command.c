#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s command [arguments...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    pid_t child = fork();
    if (child == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }
    if (child == 0) {
        execvp(argv[1], &argv[1]);
        perror("execvp");
        _exit(127);
    }

    int status;
    pid_t waited;
    do {
        waited = waitpid(child, &status, 0);
    } while (waited == -1 && errno == EINTR);
    if (waited == -1) {
        perror("waitpid");
        return EXIT_FAILURE;
    }
    if (WIFEXITED(status)) {
        int code = WEXITSTATUS(status);
        printf("Command exited with code: %d\n", code);
        return code;
    }
    if (WIFSIGNALED(status)) {
        int signal_number = WTERMSIG(status);
        printf("Command terminated by signal %d (%s)\n", signal_number,
               strsignal(signal_number));
        return 128 + signal_number;
    }
    fprintf(stderr, "Unexpected child status.\n");
    return EXIT_FAILURE;
}
