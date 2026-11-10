#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s long-text-file\n", argv[0]);
        return EXIT_FAILURE;
    }
    pid_t child = fork();
    if (child == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }
    if (child == 0) {
        execlp("cat", "cat", argv[1], (char *)NULL);
        perror("execlp cat");
        _exit(127);
    }

    printf("[PARENT] PID %ld created child %ld.\n", (long)getpid(), (long)child);
    printf("[PARENT] I can do other work while cat runs.\n");
    fflush(stdout);

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
        printf("[PARENT] Child exit code: %d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("[PARENT] Child stopped by signal: %d\n", WTERMSIG(status));
    }
    puts("[PARENT] Final line after the child has finished.");
    return WIFEXITED(status) ? WEXITSTATUS(status) : EXIT_FAILURE;
}
