A depth-limited DFS that only reports failures at exactly `depth == limit` is enough. Loop `limit` from 0 up to `MAX_SCHED`.
===
Write one function `classify(p, s)` that returns the verdict of a state using the precedence from the lesson, so every check uses the same rules.
===
The first successful call at a given limit is the answer; do not keep searching. Tid order 0, 1, 2 in the loop gives the lexicographic tie-break.
