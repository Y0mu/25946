#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node {
    char *data;
    struct Node *next;
} Node;

static int append(Node **head, Node **tail, const char *text)
{
    Node *node = malloc(sizeof(*node));

    if (node == NULL) {
        perror("malloc node");
        return -1;
    }

    /* Добавляем один байт для завершающего нуля и сохраняем независимую копию строки */
    node->data = malloc(strlen(text) + 1);
    if (node->data == NULL) {
        perror("malloc string");
        free(node);
        return -1;
    }
    strcpy(node->data, text);
    node->next = NULL;

    /* Указатель на последний узел позволяет добавлять строки без обхода всего списка */
    if (*tail == NULL) {
        *head = node;
    } else {
        (*tail)->next = node;
    }
    *tail = node;
    return 0;
}

static void free_list(Node *head)
{
    while (head != NULL) {
        /* Сохраняем следующий узел до освобождения текущего */
        Node *next = head->next;
        free(head->data);
        free(head);
        head = next;
    }
}

int main(void)
{
    char buffer[1024];
    Node *head = NULL;
    Node *tail = NULL;
    int status = EXIT_SUCCESS;

    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        size_t length;

        if (buffer[0] == '.') {
            break;
        }

        length = strlen(buffer);
        if (length > 0 && buffer[length - 1] == '\n') {
            buffer[--length] = '\0';
        } else {
            /* Отличаем строку на границе буфера от слишком длинной строки */
            int next = fgetc(stdin);
            if (next != '\n' && next != EOF) {
                fprintf(stderr, "Input line exceeds 1023 bytes\n");
                status = EXIT_FAILURE;
                break;
            }
        }

        if (length > 0 && append(&head, &tail, buffer) == -1) {
            status = EXIT_FAILURE;
            break;
        }
    }

    if (ferror(stdin)) {
        perror("read stdin");
        status = EXIT_FAILURE;
    }

    if (status == EXIT_SUCCESS) {
        for (Node *node = head; node != NULL; node = node->next) {
            puts(node->data);
        }
    }

    free_list(head);
    return status;
}
