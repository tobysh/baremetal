int main(void) {
    Job j = {7, 0};
    void *r = square_worker(&j);
    CHECK("square_worker stores input * input in output", j.output == 49, "got %d", j.output);
    CHECK("square_worker returns NULL", r == NULL, "expected NULL");

    Job neg = {-12, 0};
    square_worker(&neg);
    CHECK("square_worker handles negatives", neg.output == 144, "got %d", neg.output);

    int in1[1] = {9}, out1[1] = {-1};
    squares(in1, out1, 1);
    CHECK("squares works for one element", out1[0] == 81, "got %d", out1[0]);

    int outz[2] = {-5, -5};
    squares(in1, outz, 0);
    CHECK("squares with n = 0 leaves out untouched", outz[0] == -5 && outz[1] == -5, "out changed");

    int in8[8] = {0, 1, 2, 3, 4, 5, 6, 7}, out8[8];
    for (int i = 0; i < 8; i++) out8[i] = -1;
    squares(in8, out8, 8);
    int ok = 1;
    for (int i = 0; i < 8; i++) if (out8[i] != i * i) ok = 0;
    CHECK("squares fills every slot in order", ok, "got %d %d %d %d %d %d %d %d", out8[0], out8[1], out8[2], out8[3], out8[4], out8[5], out8[6], out8[7]);

    int in8b[8], out8b[8];
    for (int i = 0; i < 8; i++) { in8b[i] = (i - 8) * 13; out8b[i] = 0; }
    int all_ok = 1;
    for (int round = 0; round < 50; round++) {
        squares(in8b, out8b, 8);
        for (int i = 0; i < 8; i++) if (out8b[i] != in8b[i] * in8b[i]) all_ok = 0;
    }
    CHECK("squares is right with 8 threads, repeated 50 times", all_ok, "a result was wrong");
    return 0;
}
