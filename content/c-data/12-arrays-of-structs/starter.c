#include <stdio.h>

typedef struct {
    int id;
    int score;
} Entry;

int best_index(Entry e[], int n) {
    return -1;
}

int total_score(Entry e[], int n) {
    return -1;
}

int count_passing(Entry e[], int n, int pass_mark) {
    return -1;
}

void add_bonus(Entry e[], int n, int bonus) {
}

void sort_by_score(Entry e[], int n) {
}

// Optional: try them with Run.
int main(void) {
    Entry board[4] = {{1, 70}, {2, 95}, {3, 80}, {4, 95}};
    printf("best index = %d (expected 1)\n", best_index(board, 4));
    printf("total = %d (expected 340)\n", total_score(board, 4));
    sort_by_score(board, 4);
    for (int i = 0; i < 4; i++) printf("%d:%d ", board[i].id, board[i].score);
    printf("\n(expected 2:95 4:95 3:80 1:70)\n");
    return 0;
}
