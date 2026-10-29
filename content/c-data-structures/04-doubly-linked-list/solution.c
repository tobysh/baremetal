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
    l->head = NULL;
    l->tail = NULL;
    l->len = 0;
}

static Node *new_node(int v) {
    Node *n = malloc(sizeof *n);
    if (n == NULL) return NULL;
    n->val = v;
    n->prev = NULL;
    n->next = NULL;
    return n;
}

int dl_push_front(DList *l, int v) {
    Node *n = new_node(v);
    if (n == NULL) return -1;
    n->next = l->head;
    if (l->head) l->head->prev = n; else l->tail = n;
    l->head = n;
    l->len++;
    return 0;
}

int dl_insert_after(DList *l, Node *pos, int v) {
    Node *n = new_node(v);
    if (n == NULL) return -1;
    n->prev = pos;
    n->next = pos->next;
    if (pos->next) pos->next->prev = n; else l->tail = n;
    pos->next = n;
    l->len++;
    return 0;
}

int dl_push_back(DList *l, int v) {
    if (l->tail == NULL) return dl_push_front(l, v);
    return dl_insert_after(l, l->tail, v);
}

void dl_remove(DList *l, Node *n) {
    if (n->prev) n->prev->next = n->next; else l->head = n->next;
    if (n->next) n->next->prev = n->prev; else l->tail = n->prev;
    free(n);
    l->len--;
}

Node *dl_find(const DList *l, int v) {
    for (Node *n = l->head; n; n = n->next)
        if (n->val == v) return n;
    return NULL;
}

void dl_reverse(DList *l) {
    Node *n = l->head;
    while (n) {
        Node *next = n->next;
        n->next = n->prev;
        n->prev = next;
        n = next;
    }
    Node *t = l->head;
    l->head = l->tail;
    l->tail = t;
}

void dl_free(DList *l) {
    Node *n = l->head;
    while (n) {
        Node *next = n->next;
        free(n);
        n = next;
    }
    dl_init(l);
}

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
