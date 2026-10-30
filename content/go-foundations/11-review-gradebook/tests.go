package main

import (
	"errors"
	"strings"
	"testing"
)

func TestAverage_mean(t *testing.T) {
	got, err := Student{"Ann", []int{80, 90, 100}}.Average()
	if err != nil || got != 90 {
		t.Errorf("Average of 80, 90, 100 = %v, %v; want 90, nil", got, err)
	}
}

func TestAverage_not_whole(t *testing.T) {
	got, err := Student{"Bo", []int{1, 2}}.Average()
	if err != nil || got != 1.5 {
		t.Errorf("Average of 1, 2 = %v, %v; want 1.5, nil", got, err)
	}
}

func TestAverage_no_scores(t *testing.T) {
	_, err := Student{"Cy", nil}.Average()
	if ErrNoScores == nil {
		t.Fatalf("ErrNoScores is nil; create it with errors.New")
	}
	if !errors.Is(err, ErrNoScores) {
		t.Errorf("Average with no scores error = %v; want ErrNoScores", err)
	}
}

func TestAverage_bad_score_wraps_sentinel(t *testing.T) {
	for _, sc := range []int{-1, 101, 250} {
		_, err := Student{"Di", []int{50, sc}}.Average()
		if ErrBadScore == nil {
			t.Fatalf("ErrBadScore is nil; create it with errors.New")
		}
		if !errors.Is(err, ErrBadScore) {
			t.Errorf("score %d: error = %v; want it to wrap ErrBadScore", sc, err)
		}
	}
}

func TestAverage_bad_score_message(t *testing.T) {
	_, err := Student{"Di", []int{50, 120}}.Average()
	if err == nil || !strings.HasPrefix(err.Error(), "score 120: ") {
		t.Errorf("error = %v; want a message starting with %q", err, "score 120: ")
	}
}

func TestAverage_boundaries_are_valid(t *testing.T) {
	got, err := Student{"Ed", []int{0, 100}}.Average()
	if err != nil || got != 50 {
		t.Errorf("Average of 0, 100 = %v, %v; want 50, nil", got, err)
	}
}

func TestStandard_thresholds(t *testing.T) {
	cases := map[float64]string{100: "A", 90: "A", 89.9: "B", 80: "B", 79.99: "C", 70: "C", 69.5: "F", 0: "F"}
	for avg, want := range cases {
		if got := (Standard{}).Label(avg); got != want {
			t.Errorf("Standard.Label(%v) = %q; want %q", avg, got, want)
		}
	}
}

func TestStandard_is_a_Scale(t *testing.T) {
	var sc Scale = Standard{}
	if sc.Label(95) != "A" {
		t.Errorf("Scale(Standard).Label(95) = %q; want A", sc.Label(95))
	}
}

func TestReport_labels_by_name(t *testing.T) {
	students := []Student{
		{"Ann", []int{95, 90}},
		{"Bo", []int{85, 80}},
		{"Cy", []int{72, 70}},
		{"Di", []int{10, 20}},
	}
	got, err := Report(students, Standard{})
	if err != nil {
		t.Fatalf("Report error = %v; want nil", err)
	}
	want := map[string]string{"Ann": "A", "Bo": "B", "Cy": "C", "Di": "F"}
	if len(got) != len(want) {
		t.Fatalf("Report = %v; want %v", got, want)
	}
	for k, v := range want {
		if got[k] != v {
			t.Errorf("Report[%q] = %q; want %q", k, got[k], v)
		}
	}
}

func TestReport_skips_students_without_scores(t *testing.T) {
	got, err := Report([]Student{{"Ann", []int{100}}, {"Ghost", nil}}, Standard{})
	if err != nil {
		t.Fatalf("Report error = %v; want nil", err)
	}
	if _, present := got["Ghost"]; present || len(got) != 1 || got["Ann"] != "A" {
		t.Errorf("Report = %v; want only Ann: A", got)
	}
}

func TestReport_bad_data_stops_with_context(t *testing.T) {
	got, err := Report([]Student{{"Ann", []int{100}}, {"Eve", []int{50, 500}}}, Standard{})
	if got != nil {
		t.Errorf("Report map = %v; want nil when there is an error", got)
	}
	if !errors.Is(err, ErrBadScore) {
		t.Fatalf("Report error = %v; want it to wrap ErrBadScore", err)
	}
	if !strings.HasPrefix(err.Error(), "student Eve: ") {
		t.Errorf("Report error = %q; want it to start with %q", err.Error(), "student Eve: ")
	}
}

func TestReport_empty_gives_non_nil_map(t *testing.T) {
	got, err := Report(nil, Standard{})
	if err != nil || got == nil || len(got) != 0 {
		t.Errorf("Report(nil) = %v, %v; want an empty non-nil map and nil", got, err)
	}
}

type passFail struct{}

func (passFail) Label(avg float64) string {
	if avg >= 50 {
		return "pass"
	}
	return "fail"
}

func TestReport_uses_the_scale_it_is_given(t *testing.T) {
	got, err := Report([]Student{{"Ann", []int{60}}, {"Bo", []int{40}}}, passFail{})
	if err != nil || got["Ann"] != "pass" || got["Bo"] != "fail" {
		t.Errorf("Report with a custom scale = %v, %v; want Ann: pass, Bo: fail", got, err)
	}
}
