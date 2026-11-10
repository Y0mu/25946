#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Node {
    char *data;
    struct Node *next;
};

void append(struct Node **head, char *str)
{
    struct Node *new_node;
    struct Node *current;
    size_t len;

    new_node = malloc(sizeof(struct Node));

    if (new_node == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    len = strlen(str);

    new_node->data = malloc(len + 1);

    if (new_node->data == NULL) {
        perror("malloc");
        free(new_node);
        exit(EXIT_FAILURE);
    }

    strcpy(new_node->data, str);
    new_node->next = NULL;

    if (*head == NULL) {
        *head = new_node;
        return;
    }

    current = *head;

    while (current->next != NULL) {
        current = current->next;
    }

    current->next = new_node;
}

void print_list(struct Node *head)
{
    struct Node *current;

    current = head;

    while (current != NULL) {
        printf("%s\n", current->data);
        current = current->next;
    }
}

void free_list(struct Node *head)
{
    struct Node *current;
    struct Node *next;

    current = head;

    while (current != NULL) {
        next = current->next;

        free(current->data);
        free(current);

        current = next;
    }
}

int main(void)
{
    char buffer[1024];
    size_t len;
    struct Node *head;

    head = NULL;

    while (1) {
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            break;
        }

        len = strlen(buffer);

        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }

        if (buffer[0] == '.') {
            break;
        }

        if (buffer[0] != '\0') {
            append(&head, buffer);
        }
    }

    print_list(head);

    free_list(head);

    return 0;
}
