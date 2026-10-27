extern long clamp(long x, long lo, long hi);

int main(void) {
    CHECK("clamp(5, 0, 10) is 5", clamp(5, 0, 10) == 5, "got %ld", clamp(5, 0, 10));
    CHECK("clamp(-3, 0, 10) is 0", clamp(-3, 0, 10) == 0, "got %ld", clamp(-3, 0, 10));
    CHECK("clamp(42, 0, 10) is 10", clamp(42, 0, 10) == 10, "got %ld", clamp(42, 0, 10));
    CHECK("clamp(0, 0, 10) is 0 (lower bound is inclusive)", clamp(0, 0, 10) == 0, "got %ld", clamp(0, 0, 10));
    CHECK("clamp(10, 0, 10) is 10 (upper bound is inclusive)", clamp(10, 0, 10) == 10, "got %ld", clamp(10, 0, 10));
    CHECK("clamp works with negative ranges", clamp(-50, -20, -10) == -20 && clamp(-15, -20, -10) == -15 && clamp(0, -20, -10) == -10,
          "got %ld, %ld, %ld", clamp(-50, -20, -10), clamp(-15, -20, -10), clamp(0, -20, -10));
    CHECK("clamp with lo == hi always returns that value", clamp(7, 4, 4) == 4 && clamp(1, 4, 4) == 4 && clamp(4, 4, 4) == 4,
          "got %ld, %ld, %ld", clamp(7, 4, 4), clamp(1, 4, 4), clamp(4, 4, 4));
    CHECK("clamp compares as signed (-1 is below 5)", clamp(-1, 5, 9) == 5, "got %ld", clamp(-1, 5, 9));
    CHECK("clamp handles huge 64-bit values", clamp(5000000000000L, -5000000000000L, 4000000000000L) == 4000000000000L,
          "got %ld", clamp(5000000000000L, -5000000000000L, 4000000000000L));
    return 0;
}
