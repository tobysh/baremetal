#include <string.h>

static unsigned rng(unsigned *s) { *s = *s * 1103515245u + 12345u; return *s >> 16; }

/* Independent reference: one queen per row, checking every earlier queen directly. */
static int ref_go(int n, char g[9][10], int row, int *col) {
    if (row == n) return 1;
    int total = 0;
    for (int c = 0; c < n; c++) {
        if (g[row][c] == '#') continue;
        int ok = 1;
        for (int r = 0; r < row; r++) {
            int dc = col[r] - c, dr = row - r;
            if (dc == 0 || dc == dr || dc == -dr) { ok = 0; break; }
        }
        if (!ok) continue;
        col[row] = c;
        total += ref_go(n, g, row + 1, col);
    }
    return total;
}

static int reference(int n, char g[9][10]) {
    int col[9];
    return ref_go(n, g, 0, col);
}

static void make(int n, char g[9][10], const char **rows) {
    for (int r = 0; r < n; r++) { memcpy(g[r], rows[r], (size_t)n); g[r][n] = 0; }
}

int main(void) {
    char g[9][10];
    const char *rows9[9];
    for (int r = 0; r < 9; r++) rows9[r] = g[r];

    for (int n = 1; n <= 9; n++) {
        for (int r = 0; r < n; r++) { memset(g[r], '.', (size_t)n); g[r][n] = 0; }
    }
    int expect_plain[] = {0, 1, 0, 0, 2, 10, 4, 40, 92, 352};
    int bad = 0;
    for (int n = 1; n <= 9; n++) {
        for (int r = 0; r < n; r++) { memset(g[r], '.', (size_t)n); g[r][n] = 0; }
        if (count_queens_on(n, rows9) != expect_plain[n]) bad++;
    }
    CHECK("undamaged boards give the classic counts for n = 1..9", bad == 0, "%d sizes were wrong (8 queens should give 92)", bad);

    const char *four[] = { "....", "..#.", "....", "...." };
    make(4, g, four);
    CHECK("a 4x4 board with one hole matches the reference", count_queens_on(4, rows9) == reference(4, g), "got %d, expected %d", count_queens_on(4, rows9), reference(4, g));

    const char *solid[] = { "###", "###", "###" };
    make(3, g, solid);
    CHECK("a board of only holes has no solutions", count_queens_on(3, rows9) == 0, "got %d", count_queens_on(3, rows9));

    const char *deadrow[] = { "....", "####", "....", "...." };
    make(4, g, deadrow);
    CHECK("a row with no usable squares means no solutions", count_queens_on(4, rows9) == 0, "got %d", count_queens_on(4, rows9));

    const char *one[] = { "#" };
    make(1, g, one);
    CHECK("a 1x1 hole has no solution, a 1x1 square has one", count_queens_on(1, rows9) == 0, "got %d", count_queens_on(1, rows9));

    const char *holes_on_solution[] = { ".#..", "...#", "#...", "..#." };
    make(4, g, holes_on_solution);
    int want = reference(4, g);
    CHECK("holes on one solution's squares leave exactly the other", count_queens_on(4, rows9) == want && want == 1, "got %d, expected %d", count_queens_on(4, rows9), want);

    unsigned seed = 31337u;
    int mismatches = 0, nonzero = 0;
    for (int t = 0; t < 60; t++) {
        int n = 4 + (int)(rng(&seed) % 6u);
        int holes = (int)(rng(&seed) % 6u);
        for (int r = 0; r < n; r++) { memset(g[r], '.', (size_t)n); g[r][n] = 0; }
        for (int h = 0; h < holes; h++) g[rng(&seed) % (unsigned)n][rng(&seed) % (unsigned)n] = '#';
        int w = reference(n, g);
        int got = count_queens_on(n, rows9);
        if (got != w) mismatches++;
        if (w > 0) nonzero++;
    }
    CHECK("60 random damaged boards of size 4..9 match the reference", mismatches == 0, "%d boards gave a different count", mismatches);
    CHECK("the random boards were not all trivial", nonzero > 20, "internal check");

    char before[9][10];
    const char *tidy[] = { "...#.", ".....", "..#..", ".....", "....." };
    make(5, g, tidy);
    memcpy(before, g, sizeof g);
    count_queens_on(5, rows9);
    CHECK("the board is not modified", memcmp(before, g, sizeof g) == 0, "the board strings changed");
    return 0;
}
