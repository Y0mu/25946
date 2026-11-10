#include <stdio.h>        // printf, perror
#include <stdlib.h>       // exit, EXIT_FAILURE
#include <unistd.h>       // getuid(), geteuid(), setuid()
#include <sys/types.h>    // типы данных для UID
int try_open_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("fopen failed");
        return -1;
    }
    fclose(file);
    return 0;
}
int main(void) {
    printf("Real UID: %ld\n", (long)getuid());
    printf("Effective UID: %ld\n", (long)geteuid());
    if (try_open_file("data.txt") == 0) {
        printf("File opened successfully with effective UID: %ld\n", (long)geteuid());
    } else {
        printf("Failed to open file with effective UID: %ld\n", (long)geteuid());
    }

    if (setuid(getuid()) == -1) {
        perror("setuid");
        return EXIT_FAILURE;
    }
    printf("Real UID: %ld\n", (long)getuid());
    printf("Effective UID: %ld\n", (long)geteuid());

    if (try_open_file("data.txt") == 0) {
        printf("File opened successfully with effective UID: %ld\n", (long)geteuid());
    } else {
        printf("Failed to open file with effective UID: %ld\n", (long)geteuid());
    }
    return 0;
}
