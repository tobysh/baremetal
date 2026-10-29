Bug 1 is in the loop test: `i < n` must continue while `i` is strictly less than `n`. Which conditional jump does the starter use, and does it let `i == n` into the body?
===
Bug 2 is in the element comparison: the data are signed `int`s and the C says `a[i] > limit`. Look at the jump that decides whether to skip `count++`. An unsigned jump treats negative numbers as huge positive ones.
===
Bug 3 is in the epilogue: the C returns `count`, which is in `[rbp - 4]`, but the starter loads a different slot into `eax`. Match each slot to its variable using the table in the lesson.
