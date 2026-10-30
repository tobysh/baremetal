For the attack, loop over the words, add `iters` to `*work` for each attempt, and compare. Remember `*work` must be exactly attempts times `iters`, also when nothing matches.
===
`shared_hashes` is a nested loop: for each entry, look for any other index with an equal value, and count the entry if you find one.
===
`k = ceil(seconds * rate / words)`. Do the division in `double` and round up with `ceil`, or compute with integers carefully. The result is never below 1, and compare with a loop if you want to be sure about edges.
