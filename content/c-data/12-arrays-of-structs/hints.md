`best_index`, `total_score` and `count_passing` are the loops from the first lesson with `e[i].score` instead of `a[i]`. For `best_index` keep the *index* of the best so far, starting at 0, and replace it only when a score is strictly bigger (so ties keep the first).
===
`add_bonus` writes `e[i].score += bonus;` in the loop, which changes the caller's records. For `sort_by_score` use insertion sort: for each `i` from 1, save `Entry current = e[i];`, shift earlier records with a *smaller* score one place right, then drop `current` into the gap.
===
```c
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
```
