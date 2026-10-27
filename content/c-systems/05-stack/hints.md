The top of the stack is `s->items[s->size - 1]`. Push writes `s->items[s->size] = value;` and then `s->size++`. Pop does `s->size--` and reads the slot. Check for full or empty first.
===
For `is_balanced`, make a local `Stack`, call `stack_init`, and loop over the characters. Push the opener (as an int, it is a char underneath). For a closer, pop and compare with the opener that should match.
===
```c
if (c == ')' || c == ']' || c == '}') {
    int open;
    if (stack_pop(&st, &open) != 0) return 0;     // nothing to close
    char want = (c == ')') ? '(' : (c == ']') ? '[' : '{';
    if (open != want) return 0;                   // wrong kind
}
// after the loop: return st.size == 0;
```
