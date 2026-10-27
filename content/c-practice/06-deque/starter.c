#include <stdio.h>
#include <stdlib.h>

typedef struct DNode {
    int value;
    struct DNode *prev;
    struct DNode *next;
} DNode;

typedef struct {
    DNode *head;
    DNode *tail;
    int size;
} Deque;

void deque_init(Deque *d) {
}

int push_front(Deque *d, int value) {
    return -1;
}

int push_back(Deque *d, int value) {
    return -1;
}

int pop_front(Deque *d, int *out) {
    return -1;
}

int pop_back(Deque *d, int *out) {
    return -1;
}

int deque_remove(Deque *d, int value) {
    return 0;
}

void deque_reverse(Deque *d) {
}

void deque_free(Deque *d) {
}

// Optional: try them with Run.
int main(void) {
    Deque d;
    deque_init(&d);
    push_back(&d, 1);
    push_back(&d, 2);
    printf("size = %d\n", d.size);
    deque_free(&d);
    return 0;
}
