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
    Entry board[30];
    int n = 0;
    if (scanf("%d", &n) != 1 || n < 0) n = 0;
    if (n > 30) n = 30;
    for (int i = 0; i < n; i++) {
        if (scanf("%d %d", &board[i].id, &board[i].score) != 2) {
            n = i;
            break;
        }
    }
    int pass_mark = 0, bonus = 0;
    if (scanf("%d %d", &pass_mark, &bonus) != 2) {
        pass_mark = 0;
        bonus = 0;
    }

    if (n == 0) printf("best: none\n");
    else printf("best: %d\n", board[best_index(board, n)].id);
    printf("total: %d\n", total_score(board, n));
    printf("passing: %d\n", count_passing(board, n, pass_mark));
    add_bonus(board, n, bonus);
    sort_by_score(board, n);
    printf("after the bonus:\n");
    for (int i = 0; i < n; i++) printf("%d %d\n", board[i].id, board[i].score);
    return 0;
}
