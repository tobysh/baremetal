#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    int value;
    struct Node *next;
} Node;

Node *merge_sorted(Node *a, Node *b) {
    Node dummy = {0, NULL};
    Node *tail = &dummy;
    while (a != NULL && b != NULL) {
        if (b->value < a->value) {
            tail->next = b;
            b = b->next;
        } else {
            tail->next = a;
            a = a->next;
        }
        tail = tail->next;
    }
    tail->next = (a != NULL) ? a : b;
    return dummy.next;
}

// Optional: try it with Run.
int main(void) {
    printf("write merge_sorted, then press Test\n");
    return 0;
}
