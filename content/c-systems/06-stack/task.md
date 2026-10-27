`Stack` is defined in the starter (room for `STACK_MAX` ints). Implement:

- `void stack_init(Stack *s)` makes it empty.
- `int stack_push(Stack *s, int value)` returns `0` on success or `-1` if the stack is full.
- `int stack_pop(Stack *s, int *out)` stores the top value in `*out` and returns `0`, or returns `-1` if the stack is empty (leave `*out` alone).
- `int stack_peek(const Stack *s, int *out)` stores the top value in `*out` without removing it. Same return values as pop.
- `int is_balanced(const char *text)` returns `1` if every `()`, `[]` and `{}` in `text` is correctly matched and nested, otherwise `0`. Ignore all other characters. An empty string is balanced. Use your stack. If the text nests deeper than the stack can hold, return `0`.
