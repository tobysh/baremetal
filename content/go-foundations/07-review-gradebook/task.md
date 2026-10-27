Complete the gradebook:

- `ErrNoScores` and `ErrBadScore` are sentinel errors (`errors.New`).
- `(s Student) Average() (float64, error)` returns the mean of `s.Scores`. With no scores it returns `0, ErrNoScores`. If any score is below 0 or above 100 it returns `0` and an error that wraps `ErrBadScore` and starts with `"score N: "`, where `N` is the offending score.
- `Standard` implements `Scale`: `Label(avg)` returns `"A"` for 90 and above, `"B"` for 80 and above, `"C"` for 70 and above, otherwise `"F"`.
- `Report(students []Student, scale Scale) (map[string]string, error)` returns each student's name mapped to `scale.Label(average)`. Students with no scores are left out. If `Average` fails with anything other than `ErrNoScores`, return `nil` and an error that starts with `"student NAME: "` and wraps the original error. An empty input gives an empty (non-nil) map.
