#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define MAXLINES 100

struct Line {
    long offset;
    int length;
};

int main(int argc, char *argv[])
{
    int fd;
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

    while (1) {
        printf("Enter line number (0 to quit): ");
        scanf("%d", &num);

        if (num == 0)
            break;

        if (num < 1 || num > n) {
            printf("Invalid line number\n");
            continue;
        }

        lseek(fd, lines[num - 1].offset, SEEK_SET);

        buf = malloc(lines[num - 1].length + 1);

        read(fd, buf, lines[num - 1].length);

        buf[lines[num - 1].length] = '\0';

        printf("%s\n", buf);

        free(buf);
    }

    close(fd);

    return 0;
}
