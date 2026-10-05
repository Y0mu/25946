#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

static void print_uids(const char *label)
{
    printf("%s: Real UID = %d, Effective UID = %d\n",
           label, (int)getuid(), (int)geteuid());
}

static void try_open(const char *filename, const char *stage)
{
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        printf("[%s] Failed to open '%s': ", stage, filename);
        perror("");
    } else {
        printf("[%s] File '%s' opened successfully.\n", stage, filename);
        fclose(f);
    }
}

int main(void)
{
    const char *filename = "data.txt";

    printf("=== Program started ===\n");

    print_uids("Before dropping privileges");
    try_open(filename, "Before drop");

    printf("\n>>> Calling setuid(getuid())...\n\n");
    if (setuid(getuid()) != 0) {
        perror("setuid");
        return EXIT_FAILURE;
    }

    print_uids("After dropping privileges");
    try_open(filename, "After drop");

    printf("=== Program finished ===\n");
    return 0;
}
