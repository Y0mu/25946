#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

void print_ids(void)
{
    printf("Real UID: %d\n", (int)getuid());
    printf("Effective UID: %d\n", (int)geteuid());
}

void try_open_file(void)
{
    FILE *f;

    f = fopen("data.txt", "r");

    if (f == NULL) {
        perror("fopen");
        return;
    }

    printf("data.txt opened successfully\n");
    fclose(f);
}

int main(void)
{
    printf("Before setuid:\n");
    print_ids();
    try_open_file();

    if (setuid(getuid()) == -1) {
        perror("setuid");
        return EXIT_FAILURE;
    }

    printf("\nAfter setuid(getuid()):\n");
    print_ids();
    try_open_file();

    return 0;
}
