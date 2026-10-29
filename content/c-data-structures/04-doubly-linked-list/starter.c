#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    int val;
    struct Node *prev;
    struct Node *next;
} Node;

typedef struct {
    Node *head;
    Node *tail;
    size_t len;
} DList;

void dl_init(DList *l) {
}

int dl_push_back(DList *l, int v) {
    return -1;
}

int dl_push_front(DList *l, int v) {
    return -1;
}

int dl_insert_after(DList *l, Node *pos, int v) {
    return -1;
}

void dl_remove(DList *l, Node *n) {
}

Node *dl_find(const DList *l, int v) {
    return NULL;
}

void dl_reverse(DList *l) {
}

void dl_free(DList *l) {
}

// Optional: try it with Run.
int main(void) {
    DList l;
    dl_init(&l);
    dl_push_back(&l, 1);
    dl_push_back(&l, 2);
    dl_push_front(&l, 0);
    for (Node *n = l.head; n; n = n->next) printf("%d ", n->val);
    printf("\n");
    dl_free(&l);
    return 0;
}
