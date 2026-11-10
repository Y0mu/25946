#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Node {
    char *text;
    struct Node *next;
};

/* fgets is used in chunks so long input lines are not silently split. */
static char *read_line(void)
{
    size_t capacity = 1024;
    size_t length = 0;
    char *line = malloc(capacity);
    if (line == NULL)
        return NULL;

    line[0] = '\0';
    for (;;) {
        if (fgets(line + length, (int)(capacity - length), stdin) == NULL) {
            if (length == 0) {
                free(line);
                return NULL;
            }
            break;
        }
        length += strlen(line + length);
        if (length > 0 && line[length - 1] == '\n') {
            line[--length] = '\0';
            break;
        }
        if (feof(stdin))
            break;
        if (capacity > (size_t)-1 / 2) {
            free(line);
            return NULL;
        }
        capacity *= 2;
        char *larger = realloc(line, capacity);
        if (larger == NULL) {
            free(line);
            return NULL;
        }
        line = larger;
    }
    return line;
}

static void free_list(struct Node *head)
{
    while (head != NULL) {
        struct Node *next = head->next;
        free(head->text);
        free(head);
        head = next;
    }
}

int main(void)
{
    struct Node *head = NULL;
    struct Node **tail = &head;
    int result = EXIT_SUCCESS;

    for (;;) {
        char *line = read_line();
        if (line == NULL) {
            if (!feof(stdin)) {
                perror("reading input");
                result = EXIT_FAILURE;
            }
            break;
        }
        if (line[0] == '.') {
            free(line);
            break;
        }
        struct Node *node = malloc(sizeof(*node));
        if (node == NULL) {
            perror("malloc");
            free(line);
            result = EXIT_FAILURE;
            break;
        }
        node->text = line;
        node->next = NULL;
        *tail = node;
        tail = &node->next;
    }

    for (struct Node *node = head; node != NULL; node = node->next)
        puts(node->text);
    free_list(head);
    return result;
}
