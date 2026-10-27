package main

import (
	"errors"
	"fmt"
)

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

var ErrNoScores = errors.New("no scores")
var ErrBadScore = errors.New("score out of range")

func (s Student) Average() (float64, error) {
	if len(s.Scores) == 0 {
		return 0, ErrNoScores
	}
	total := 0
	for _, sc := range s.Scores {
		if sc < 0 || sc > 100 {
			return 0, fmt.Errorf("score %d: %w", sc, ErrBadScore)
		}
		total += sc
	}
	return float64(total) / float64(len(s.Scores)), nil
}

func (Standard) Label(avg float64) string {
	switch {
	case avg >= 90:
		return "A"
	case avg >= 80:
		return "B"
	case avg >= 70:
		return "C"
	default:
		return "F"
	}
}

func Report(students []Student, scale Scale) (map[string]string, error) {
	out := make(map[string]string)
	for _, s := range students {
		avg, err := s.Average()
		if errors.Is(err, ErrNoScores) {
			continue
		}
		if err != nil {
			return nil, fmt.Errorf("student %s: %w", s.Name, err)
		}
		out[s.Name] = scale.Label(avg)
	}
	return out, nil
}
