`word_freq(text, out, cap)` counts how often each word occurs and writes `word=count` lines, in order of first appearance. It already produces the right output. It does too much work to get there.

- Run the program and read the profile table. Find the stage that does the most work **by the counters**, not by what looks wasteful.
- Fix that stage so it makes at most 4 comparisons per input word on average, whatever the input.
- The output must stay byte-for-byte identical, including the order of words.
- Do not change the counter block, `same`, `dup_word` or the other stages' accounting. Every comparison of words must still go through `same(ST_COUNT, ...)`.

Several workloads are tried: empty text, extra spaces, one word repeated, all words different, and large mixed texts. The check reads the counters, never a clock.
