#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

typedef struct Node {
    int value;
    struct Node *next;
} Node;

Node *push_front(Node *head, int value) {
    Node *n = malloc(sizeof *n);
    if (n == NULL) {
        return head;
    }
    n->value = value;
    n->next = head;
    return n;
}

size_t list_length(const Node *head) {
    size_t count = 0;
    for (const Node *cur = head; cur != NULL; cur = cur->next) {
        count++;
    }
    return count;
}

Node *reverse_list(Node *head) {
    Node *prev = NULL;
    Node *cur = head;
    while (cur != NULL) {
        Node *next = cur->next;
        cur->next = prev;
        prev = cur;
        cur = next;
    }
    return prev;
}

void free_list(Node *head) {
    while (head != NULL) {
        Node *next = head->next;
        free(head);
        head = next;
    }
}

int main(void) {
    Node *list = NULL;
    for (int i = 1; i <= 3; i++) list = push_front(list, i);
    for (const Node *n = list; n != NULL; n = n->next) printf("%d ", n->value);
    printf("\n");
    free_list(list);
    return 0;
}
