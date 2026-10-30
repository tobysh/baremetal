Compare the `compares`, `allocs` and `copied` columns across the three stages. Which single number is a hundred times bigger than the rest?
===
`tokenize` allocates once per word, which looks wasteful, but the table shows it costs 2000 allocations and about 9 KB of copying. `count_words` compares each word with every distinct word seen so far.
===
You need to find a word among the ones already seen without looking at all of them. Hash the word into a table of chains, then only compare against words in the same chain, using `same(ST_COUNT, ...)`.
