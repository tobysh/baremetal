#include <stdio.h>

#define STACK_MAX 32

typedef struct {
    int items[STACK_MAX];
    int size;
} Stack;

void stack_init(Stack *s) {
    s->size = 0;
}

int stack_push(Stack *s, int value) {
    if (s->size == STACK_MAX) {
        return -1;
    }
    s->items[s->size] = value;
    s->size++;
    return 0;
}

int stack_pop(Stack *s, int *out) {
    if (s->size == 0) {
        return -1;
    }
    s->size--;
    *out = s->items[s->size];
    return 0;
}

int stack_peek(const Stack *s, int *out) {
    if (s->size == 0) {
        return -1;
    }
    *out = s->items[s->size - 1];
    return 0;
}

int is_balanced(const char *text) {
    Stack st;
    stack_init(&st);
    for (int i = 0; text[i] != '\0'; i++) {
        char c = text[i];
        if (c == '(' || c == '[' || c == '{') {
            if (stack_push(&st, c) != 0) {
                return 0;
            }
        } else if (c == ')' || c == ']' || c == '}') {
            int open;
            if (stack_pop(&st, &open) != 0) {
                return 0;
            }
            char want = (c == ')') ? '(' : (c == ']') ? '[' : '{';
            if (open != want) {
                return 0;
            }
        }
    }
    return st.size == 0;
}

int main(void) {
    printf("%d\n", is_balanced("{[()]}"));
    return 0;
}
