#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node {
    int value;
    struct Node *next;
} Node;

static Node *push_front(Node *head, int value) {
    Node *n = malloc(sizeof *n);
    if (n == NULL) return head;
    n->value = value;
    n->next = head;
    return n;
}

static Node *reverse_list(Node *head) {
    Node *prev = NULL;
    while (head != NULL) {
        Node *next = head->next;
        head->next = prev;
        prev = head;
        head = next;
    }
    return prev;
}

static size_t list_length(const Node *head) {
    size_t count = 0;
    for (; head != NULL; head = head->next) count++;
    return count;
}

static void free_list(Node *head) {
    while (head != NULL) {
        Node *next = head->next;
        free(head);
        head = next;
    }
}

int main(void) {
    Node *head = NULL;
    char cmd[16];
    while (scanf("%15s", cmd) == 1) {
        if (strcmp(cmd, "push") == 0) {
            int v;
            if (scanf("%d", &v) != 1) break;
            head = push_front(head, v);
        } else if (strcmp(cmd, "pop") == 0) {
            if (head == NULL) {
                printf("empty\n");
            } else {
                Node *old = head;
                printf("%d\n", old->value);
                head = old->next;
                free(old);
            }
        } else if (strcmp(cmd, "reverse") == 0) {
            head = reverse_list(head);
        } else if (strcmp(cmd, "print") == 0) {
            if (head == NULL) printf("empty\n");
            for (const Node *cur = head; cur != NULL; cur = cur->next) printf("%d%c", cur->value, cur->next ? ' ' : '\n');
        } else if (strcmp(cmd, "count") == 0) {
            printf("%zu\n", list_length(head));
        }
    }
    free_list(head);
    return 0;
}
