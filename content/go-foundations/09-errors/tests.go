package main

import (
	"errors"
	"strconv"
	"strings"
	"testing"
)

func TestDivide_ok(t *testing.T) {
	got, err := Divide(9, 3)
	if err != nil || got != 3 {
		t.Errorf("Divide(9, 3) = %v, %v; want 3, nil", got, err)
	}
}

func TestDivide_by_zero_returns_the_sentinel(t *testing.T) {
	_, err := Divide(1, 0)
	if ErrDivideByZero == nil {
		t.Fatalf("ErrDivideByZero is nil; create it with errors.New")
	}
	if !errors.Is(err, ErrDivideByZero) {
		t.Errorf("Divide(1, 0) error = %v; want ErrDivideByZero", err)
	}
}

func TestRangeError_message(t *testing.T) {
	var err error = &RangeError{Value: 200, Min: 0, Max: 150}
	if got := err.Error(); got != "200 is outside 0..150" {
		t.Errorf("RangeError.Error() = %q; want %q", got, "200 is outside 0..150")
	}
}

func TestParseAge_valid(t *testing.T) {
	for in, want := range map[string]int{"0": 0, "42": 42, "150": 150} {
		got, err := ParseAge(in)
		if err != nil || got != want {
			t.Errorf("ParseAge(%q) = %d, %v; want %d, nil", in, got, err, want)
		}
	}
}

func TestParseAge_not_a_number_wraps_the_strconv_error(t *testing.T) {
	_, err := ParseAge("abc")
	if err == nil {
		t.Fatalf("ParseAge(\"abc\") returned a nil error")
	}
	if !strings.HasPrefix(err.Error(), "parse age: ") {
		t.Errorf("error message = %q; want it to start with \"parse age: \"", err.Error())
	}
	var numErr *strconv.NumError
	if !errors.As(err, &numErr) {
		t.Errorf("errors.As(err, *strconv.NumError) is false; wrap the original error with %%w")
	}
}

func TestParseAge_out_of_range_is_a_RangeError(t *testing.T) {
	for _, in := range []string{"-1", "151", "9999"} {
		_, err := ParseAge(in)
		var re *RangeError
		if !errors.As(err, &re) {
			t.Errorf("ParseAge(%q) error = %v; want a *RangeError", in, err)
			continue
		}
		if re.Min != 0 || re.Max != 150 {
			t.Errorf("ParseAge(%q) RangeError = %+v; want Min 0, Max 150", in, *re)
		}
	}
}
