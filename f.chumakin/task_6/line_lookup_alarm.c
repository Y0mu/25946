#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#define MAXLINES 100

struct Line {
    long offset;
    int length;
};

int fd;

void alarm_handler(int sig)
{
    char buf[256];
    int n;

    printf("\n[TIMEOUT] Time is up! Printing full file content...\n");

    lseek(fd, 0, SEEK_SET);

    while ((n = read(fd, buf, sizeof(buf))) > 0)
        write(STDOUT_FILENO, buf, n);

    close(fd);
    _exit(0);
}

int main(int argc, char *argv[])
{
    char ch;
    struct Line lines[MAXLINES];
    int n = 0;
    int len = 0;
    long start = 0;
    int num;
    char *buf;

    if (argc != 2) {
        printf("Usage: %s file\n", argv[0]);
        return 1;
    }

    fd = open(argv[1], O_RDONLY);

    if (fd == -1) {
        perror("open");
        return 1;
    }

    while (read(fd, &ch, 1) > 0) {
        if (ch == '\n') {
            lines[n].offset = start;
            lines[n].length = len;
            n++;

            start = lseek(fd, 0, SEEK_CUR);
            len = 0;
        } else {
            len++;
        }
    }

    if (len > 0) {
        lines[n].offset = start;
        lines[n].length = len;
        n++;
    }

    signal(SIGALRM, alarm_handler);

    while (1) {
        printf("Enter line number (0 to quit, 5 sec timeout): ");
        fflush(stdout);

        alarm(5);

        if (scanf("%d", &num) != 1) {
            alarm(0);
            break;
        }

        alarm(0);

        if (num == 0)
            break;

        if (num < 1 || num > n) {
            printf("Invalid line number\n");
            continue;
        }

        lseek(fd, lines[num - 1].offset, SEEK_SET);

        buf = malloc(lines[num - 1].length + 1);

        if (buf == NULL) {
            perror("malloc");
            break;
        }

        read(fd, buf, lines[num - 1].length);

        buf[lines[num - 1].length] = '\0';

        printf("%s\n", buf);

        free(buf);
    }

    close(fd);

    return 0;
}
