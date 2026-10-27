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
    d->head = NULL;
    d->tail = NULL;
    d->size = 0;
}

int push_front(Deque *d, int value) {
    DNode *n = malloc(sizeof(DNode));
    if (n == NULL) {
        return -1;
    }
    n->value = value;
    n->prev = NULL;
    n->next = d->head;
    if (d->head != NULL) {
        d->head->prev = n;
    } else {
        d->tail = n;
    }
    d->head = n;
    d->size++;
    return 0;
}

int push_back(Deque *d, int value) {
    DNode *n = malloc(sizeof(DNode));
    if (n == NULL) {
        return -1;
    }
    n->value = value;
    n->next = NULL;
    n->prev = d->tail;
    if (d->tail != NULL) {
        d->tail->next = n;
    } else {
        d->head = n;
    }
    d->tail = n;
    d->size++;
    return 0;
}

int pop_front(Deque *d, int *out) {
    if (d->head == NULL) {
        return -1;
    }
    DNode *n = d->head;
    *out = n->value;
    d->head = n->next;
    if (d->head != NULL) {
        d->head->prev = NULL;
    } else {
        d->tail = NULL;
    }
    free(n);
    d->size--;
    return 0;
}

int pop_back(Deque *d, int *out) {
    if (d->tail == NULL) {
        return -1;
    }
    DNode *n = d->tail;
    *out = n->value;
    d->tail = n->prev;
    if (d->tail != NULL) {
        d->tail->next = NULL;
    } else {
        d->head = NULL;
    }
    free(n);
    d->size--;
    return 0;
}

int deque_remove(Deque *d, int value) {
    for (DNode *n = d->head; n != NULL; n = n->next) {
        if (n->value == value) {
            if (n->prev != NULL) {
                n->prev->next = n->next;
            } else {
                d->head = n->next;
            }
            if (n->next != NULL) {
                n->next->prev = n->prev;
            } else {
                d->tail = n->prev;
            }
            free(n);
            d->size--;
            return 1;
        }
    }
    return 0;
}

void deque_reverse(Deque *d) {
    DNode *n = d->head;
    while (n != NULL) {
        DNode *next = n->next;
        n->next = n->prev;
        n->prev = next;
        n = next;
    }
    DNode *tmp = d->head;
    d->head = d->tail;
    d->tail = tmp;
}

void deque_free(Deque *d) {
    DNode *n = d->head;
    while (n != NULL) {
        DNode *next = n->next;
        free(n);
        n = next;
    }
    deque_init(d);
}

int main(void) {
    Deque d;
    deque_init(&d);
    push_back(&d, 1);
    push_back(&d, 2);
    printf("size = %d\n", d.size);
    deque_free(&d);
    return 0;
}
