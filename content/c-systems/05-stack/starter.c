#include <stdio.h>

#define STACK_MAX 32

typedef struct {
    int items[STACK_MAX];
    int size;
} Stack;

void stack_init(Stack *s) {
}

int stack_push(Stack *s, int value) {
    return -1;
}

int stack_pop(Stack *s, int *out) {
    return -1;
}

int stack_peek(const Stack *s, int *out) {
    return -1;
}

int is_balanced(const char *text) {
    return 0;
}

// Optional: try them with Run.
int main(void) {
    printf("%d\n", is_balanced("{[()]}"));
    return 0;
}
