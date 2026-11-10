Start with a struct holding the id and the score, an array of them, and a loop that reads the entries. Print what you read before doing anything else.
===
Split the report into small functions that take the array and its length: one finds the index of the best entry, one adds the scores, one counts the passes, one adds the bonus. Calling them from `main` in the order of the report keeps the output order right.
===
For the best entry, remember the index of the best so far and replace it only when a score is strictly higher, so a tie keeps the first.
===
Sort with insertion sort: take `current = e[i]`, shift entries with a lower score one place right, then drop `current` into the gap. Only shift past strictly lower scores, so ties stay in order. Print `best`, `total` and `passing` before you add the bonus.
