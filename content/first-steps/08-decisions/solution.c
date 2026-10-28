int max_of_three(int a, int b, int c) {
    int best = a;
    if (b > best) best = b;
    if (c > best) best = c;
    return best;
}

int is_even(int n) {
    return n % 2 == 0;
}
