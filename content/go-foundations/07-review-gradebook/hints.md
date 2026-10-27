Create the sentinels with `errors.New("...")`. In `Average`, check `len(s.Scores) == 0` first and return `0, ErrNoScores`. Then loop with `range`, reject out-of-range scores with `fmt.Errorf("score %d: %w", sc, ErrBadScore)`, and divide the total by the count after converting both to `float64`.
===
`Label` is a `switch` with no tag (`switch { case avg >= 90: ... }`), checked from the highest threshold down, with `default` returning `"F"`.
===
In `Report`, make the map with `make(map[string]string)`, then for each student call `Average`. If `errors.Is(err, ErrNoScores)`, `continue`. Any other non-nil error is returned as `nil, fmt.Errorf("student %s: %w", s.Name, err)`. Otherwise store `scale.Label(avg)` under the student's name.
