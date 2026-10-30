**Challenge: group the anagrams.**

Write `GroupAnagrams(words []string) [][]string`. Two words are anagrams when they use exactly the same letters (`"listen"` and `"silent"`). Return the words grouped by anagram class:

- groups appear in the order their **first word** appeared in the input;
- inside a group, words keep their input order;
- the result for empty input is an empty, non-nil slice;
- compare letters case-sensitively, exactly as given.

A map from a *canonical key* to a group index does the job. The key can be the word's letters sorted (`sort.Slice` on a `[]rune`, or `slices.Sort`, then `string(...)`).
