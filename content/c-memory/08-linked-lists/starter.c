#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

typedef struct Node {
    int value;
    struct Node *next;
} Node;

Node *push_front(Node *head, int value) {
    return NULL;
}

size_t list_length(const Node *head) {
    return 0;
}

Node *reverse_list(Node *head) {
    return NULL;
}

void free_list(Node *head) {
}

// Optional: try them with Run.
int main(void) {
    Node *list = NULL;
    for (int i = 1; i <= 3; i++) list = push_front(list, i);
    for (const Node *n = list; n != NULL; n = n->next) printf("%d ", n->value);
    printf("\n(expected 3 2 1)\n");
    free_list(list);
    return 0;
}
