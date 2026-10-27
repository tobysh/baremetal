int main(void) {
    Stack s;
    stack_init(&s);
    int out = -1;
    CHECK("a new stack is empty, so pop fails", stack_pop(&s, &out) == -1 && out == -1, "pop did not fail on an empty stack");
    CHECK("a new stack is empty, so peek fails", stack_peek(&s, &out) == -1 && out == -1, "peek did not fail on an empty stack");

    CHECK("push succeeds", stack_push(&s, 10) == 0 && stack_push(&s, 20) == 0 && stack_push(&s, 30) == 0, "push failed");
    CHECK("peek shows the top without removing it", stack_peek(&s, &out) == 0 && out == 30 && s.size == 3, "out = %d, size = %d", out, s.size);

    int a = 0, b = 0, c = 0;
    int r1 = stack_pop(&s, &a);
    int r2 = stack_pop(&s, &b);
    int r3 = stack_pop(&s, &c);
    CHECK("pops come out in reverse order (last in, first out)", r1 == 0 && r2 == 0 && r3 == 0 && a == 30 && b == 20 && c == 10, "got %d %d %d", a, b, c);
    CHECK("the stack is empty again after popping everything", stack_pop(&s, &out) == -1, "pop should fail");

    stack_init(&s);
    int ok = 1;
    for (int i = 0; i < STACK_MAX; i++) if (stack_push(&s, i) != 0) ok = 0;
    CHECK("the stack accepts STACK_MAX items", ok, "a push failed too early");
    CHECK("pushing onto a full stack returns -1", stack_push(&s, 999) == -1, "push should fail when full");
    CHECK("a failed push leaves the top unchanged", stack_peek(&s, &out) == 0 && out == STACK_MAX - 1, "top = %d", out);

    CHECK("\"\" is balanced", is_balanced("") == 1, "expected 1");
    CHECK("\"()\" is balanced", is_balanced("()") == 1, "expected 1");
    CHECK("\"{[()]}\" is balanced", is_balanced("{[()]}") == 1, "expected 1");
    CHECK("\"()[]{}\" is balanced", is_balanced("()[]{}") == 1, "expected 1");
    CHECK("other characters are ignored", is_balanced("if (a[i] > 0) { run(); }") == 1, "expected 1");
    CHECK("\"(]\" is not balanced (wrong kind)", is_balanced("(]") == 0, "expected 0");
    CHECK("\"([)]\" is not balanced (bad nesting)", is_balanced("([)]") == 0, "expected 0");
    CHECK("\"((\" is not balanced (never closed)", is_balanced("((") == 0, "expected 0");
    CHECK("\")(\" is not balanced (closer first)", is_balanced(")(") == 0, "expected 0");
    CHECK("\"a)\" is not balanced (nothing to close)", is_balanced("a)") == 0, "expected 0");

    char deep[80];
    for (int i = 0; i < 40; i++) deep[i] = '(';
    for (int i = 40; i < 79; i++) deep[i] = ')';
    deep[79] = '\0';
    CHECK("nesting deeper than the stack does not crash", is_balanced(deep) == 0, "expected 0");
    return 0;
}
