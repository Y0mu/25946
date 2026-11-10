#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

void print_ids() {
    printf("Real UID: %d\n", (int)getuid());
    printf("Effective UID: %d\n", (int)geteuid());
}

void open_file() {
    FILE *file = fopen("data.txt", "r");

    if (file == NULL) {
        printf("Cannot open data.txt: %s\n", strerror(errno));
    } else {
        printf("data.txt opened successfully\n");
        fclose(file);
    }
}

int main() 
{
    printf("Before setuid:\n");
    print_ids();
    open_file();

    if (setuid(getuid()) == -1) {
        printf("setuid error: %s\n", strerror(errno));
        return 1;
    }

    printf("\nAfter setuid:\n");
    print_ids();
    open_file();

    return 0;
}
