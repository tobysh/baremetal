package main

type Student struct {
	Name   string
	Scores []int
}

// Scale turns an average score into a label.
type Scale interface {
	Label(avg float64) string
}

// Standard is the usual A/B/C/F scale.
type Standard struct{}

// ErrNoScores and ErrBadScore should be sentinel errors (see errors.New).
var ErrNoScores error
var ErrBadScore error

// Average returns the mean score, ErrNoScores for no scores, or an error
// wrapping ErrBadScore for a score outside 0..100.
func (s Student) Average() (float64, error) {
	return 0, nil
}

func (Standard) Label(avg float64) string {
	return ""
}

// Report maps each student's name to a label from scale.
func Report(students []Student, scale Scale) (map[string]string, error) {
	return nil, nil
}
