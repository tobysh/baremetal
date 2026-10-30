`strings.Fields` splits on whitespace and `strings.ToLower` lowers each word (add `import "strings"` at the top of the file). A loop with `counts[w]++` does the counting.
===
For `Dedup`, keep a `seen := make(map[int]bool)`. For each number, if `!seen[n]` then mark it and `append` it to a result slice.
===
Create the result with `result := []int{}` (or `make([]int, 0, len(nums))`) and return it, so empty input returns an empty slice instead of nil. Don't write into `nums`.
