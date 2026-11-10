Write a program that reports on a scoreboard. The input starts with the number of entries (at most 30). Each entry is a line holding an id and a score, both integers. After the entries comes one last line with a pass mark and a bonus.

The report has these parts, in order:

- `best:` and the id of the entry with the highest score. If several share it, the one entered first. With no entries there is no best, so print `best: none`.
- `total:` and the sum of all the scores.
- `passing:` and how many entries scored at least the pass mark. All of these use the scores as entered, before the bonus.
- A line `after the bonus:`, then every entry as `id score` with the bonus added to its score, one per line, from the highest score to the lowest. Entries with equal scores keep the order they were entered in.

For the input

```
4
1 70
2 95
3 80
4 95
80 5
```

the output is:

```
best: 2
total: 340
passing: 3
after the bonus:
2 100
4 100
3 85
1 75
```

Bonuses can be negative. Other scoreboards are tried, including an empty one. Start from the empty file: choose your own struct and functions.
