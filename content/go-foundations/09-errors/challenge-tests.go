package main

import (
	"errors"
	"reflect"
	"strconv"
	"strings"
	"testing"
)

func TestParseAges_all_valid(t *testing.T) {
	ages, err := ParseAges([]string{"30", "0", "150"})
	if err != nil {
		t.Fatalf("ParseAges returned %v; want nil when every input is valid", err)
	}
	if !reflect.DeepEqual(ages, []int{30, 0, 150}) {
		t.Errorf("ages = %v; want [30 0 150]", ages)
	}
}

func TestParseAges_empty_input(t *testing.T) {
	ages, err := ParseAges(nil)
	if err != nil || len(ages) != 0 {
		t.Errorf("ParseAges(nil) = %v, %v; want no ages and a nil error", ages, err)
	}
}

func TestParseAges_keeps_valid_ages_and_joins_failures(t *testing.T) {
	ages, err := ParseAges([]string{"30", "x", "200", "41"})
	if !reflect.DeepEqual(ages, []int{30, 41}) {
		t.Errorf("ages = %v; want [30 41] (valid ones, in order)", ages)
	}
	if err == nil {
		t.Fatalf("expected an error for inputs 1 and 2")
	}
	lines := strings.Split(err.Error(), "\n")
	if len(lines) != 2 {
		t.Fatalf("error text has %d lines; want 2 (one per failure, use errors.Join): %q", len(lines), err.Error())
	}
	if !strings.HasPrefix(lines[0], "input 1: parse age: ") {
		t.Errorf("first line = %q; want it to start with %q", lines[0], "input 1: parse age: ")
	}
	if lines[1] != "input 2: 200 is outside 0..150" {
		t.Errorf("second line = %q; want %q", lines[1], "input 2: 200 is outside 0..150")
	}
}

func TestParseAges_errors_are_still_inspectable(t *testing.T) {
	_, err := ParseAges([]string{"7", "abc", "-4"})
	if !errors.Is(err, strconv.ErrSyntax) {
		t.Errorf("errors.Is(err, strconv.ErrSyntax) is false; wrap with %%w so the cause stays visible")
	}
	var re *RangeError
	if !errors.As(err, &re) {
		t.Fatalf("errors.As did not find a *RangeError")
	}
	if re.Value != -4 {
		t.Errorf("RangeError.Value = %d; want -4", re.Value)
	}
}

func TestParseAges_huge_number_is_a_range_problem_from_strconv(t *testing.T) {
	ages, err := ParseAges([]string{"99999999999999999999999"})
	if len(ages) != 0 {
		t.Errorf("ages = %v; want none", ages)
	}
	if !errors.Is(err, strconv.ErrRange) {
		t.Errorf("errors.Is(err, strconv.ErrRange) is false for err = %v", err)
	}
}

func TestRangeErrors_finds_all_of_them(t *testing.T) {
	_, err := ParseAges([]string{"151", "ok", "-1", "20", "400"})
	got := RangeErrors(err)
	var values []int
	for _, r := range got {
		values = append(values, r.Value)
	}
	if !reflect.DeepEqual(values, []int{151, -1, 400}) {
		t.Errorf("RangeErrors found values %v; want [151 -1 400] in order", values)
	}
}

func TestRangeErrors_nested_and_plain(t *testing.T) {
	if got := RangeErrors(nil); len(got) != 0 {
		t.Errorf("RangeErrors(nil) = %v; want empty", got)
	}
	if got := RangeErrors(ErrDivideByZero); len(got) != 0 {
		t.Errorf("RangeErrors(ErrDivideByZero) = %v; want empty", got)
	}
	a, b := &RangeError{Value: 1, Min: 5, Max: 9}, &RangeError{Value: 99, Min: 5, Max: 9}
	nested := errors.Join(ErrDivideByZero, errors.Join(a, errors.New("noise")), b)
	got := RangeErrors(nested)
	if len(got) != 2 || got[0] != a || got[1] != b {
		t.Errorf("RangeErrors(nested) = %v; want exactly the two RangeError pointers a then b", got)
	}
	_, err := ParseAges([]string{"1000"})
	one := RangeErrors(err)
	if len(one) != 1 || one[0].Value != 1000 {
		t.Errorf("RangeErrors found %v; want one RangeError with Value 1000", one)
	}
}
