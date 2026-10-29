#include <stdio.h>

typedef struct {
    int id;
    int score;
} Entry;

int best_index(Entry e[], int n) {
    int best = 0;
    for (int i = 1; i < n; i++) {
        if (e[i].score > e[best].score) best = i;
    }
    return best;
}

int total_score(Entry e[], int n) {
    int total = 0;
    for (int i = 0; i < n; i++) {
        total += e[i].score;
    }
    return total;
}

int count_passing(Entry e[], int n, int pass_mark) {
    int count = 0;
    for (int i = 0; i < n; i++) {
        if (e[i].score >= pass_mark) count++;
    }
    return count;
}

void add_bonus(Entry e[], int n, int bonus) {
    for (int i = 0; i < n; i++) {
        e[i].score += bonus;
    }
}

void sort_by_score(Entry e[], int n) {
    for (int i = 1; i < n; i++) {
        Entry current = e[i];
        int j = i - 1;
        while (j >= 0 && e[j].score < current.score) {
            e[j + 1] = e[j];
            j--;
        }
        e[j + 1] = current;
    }
}

int main(void) {
    Entry board[4] = {{1, 70}, {2, 95}, {3, 80}, {4, 95}};
    printf("best index = %d\n", best_index(board, 4));
    printf("total = %d\n", total_score(board, 4));
    sort_by_score(board, 4);
    for (int i = 0; i < 4; i++) printf("%d:%d ", board[i].id, board[i].score);
    printf("\n");
    return 0;
}
