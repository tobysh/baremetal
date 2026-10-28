package main

import (
	"errors"
	"fmt"
	"testing"
)

func TestLookup_found(t *testing.T) {
	v, err := Lookup(map[string]int{"a": 7}, "a")
	if v != 7 || err != nil {
		t.Errorf("Lookup(a) = %d, %v; want 7, nil", v, err)
	}
}

func TestLookup_missing_wraps_the_sentinel(t *testing.T) {
	v, err := Lookup(map[string]int{}, "zed")
	if v != 0 || err == nil {
		t.Fatalf("Lookup(zed) = %d, %v; want 0 and an error", v, err)
	}
	if err == ErrNotFound {
		t.Errorf("the error must add context, not be the sentinel itself")
	}
	if !errors.Is(err, ErrNotFound) {
		t.Errorf("errors.Is(err, ErrNotFound) = false; wrap with %%w, not %%v")
	}
	if err.Error() != `lookup "zed": not found` {
		t.Errorf("error text = %q; want %q", err.Error(), `lookup "zed": not found`)
	}
}

func TestValidationError_text(t *testing.T) {
	e := &ValidationError{Field: "x", Reason: "bad"}
	if e.Error() != "x: bad" {
		t.Errorf("Error() = %q; want \"x: bad\"", e.Error())
	}
}

func TestValidate_cases(t *testing.T) {
	if err := Validate("Ada", 36); err != nil {
		t.Errorf("Validate(Ada, 36) = %v; want nil", err)
	}
	var ve *ValidationError
	err := Validate("", 36)
	if !errors.As(err, &ve) || ve.Field != "name" || ve.Reason != "must not be empty" {
		t.Errorf("Validate(\"\", 36) = %#v; want a *ValidationError for name", err)
	}
	err = Validate("Ada", 151)
	ve = nil
	if !errors.As(err, &ve) || ve.Field != "age" || ve.Reason != "must be between 0 and 150" {
		t.Errorf("Validate(Ada, 151) = %#v; want a *ValidationError for age", err)
	}
	if err := Validate("Ada", -1); err == nil {
		t.Errorf("Validate(Ada, -1) = nil; want an error")
	}
	if err := Validate("Ada", 150); err != nil {
		t.Errorf("Validate(Ada, 150) = %v; 150 is allowed", err)
	}
	ve = nil
	if err := Validate("", 500); !errors.As(err, &ve) || ve.Field != "name" {
		t.Errorf("when both are wrong the name is reported first, got %v", err)
	}
}

func TestDescribe_each_kind(t *testing.T) {
	_, missing := Lookup(nil, "k")
	cases := []struct {
		err  error
		want string
	}{
		{nil, "ok"},
		{missing, "missing"},
		{Validate("", 1), "invalid name (must not be empty)"},
		{Validate("a", 200), "invalid age (must be between 0 and 150)"},
		{errors.New("disk on fire"), "error: disk on fire"},
	}
	for _, c := range cases {
		if got := Describe(c.err); got != c.want {
			t.Errorf("Describe(%v) = %q; want %q", c.err, got, c.want)
		}
	}
}

func TestDescribe_sees_through_extra_wrapping(t *testing.T) {
	_, missing := Lookup(nil, "k")
	wrapped := fmt.Errorf("loading config: %w", fmt.Errorf("step 2: %w", missing))
	if got := Describe(wrapped); got != "missing" {
		t.Errorf("Describe(double-wrapped not-found) = %q; want \"missing\" (use errors.Is)", got)
	}
	wrappedV := fmt.Errorf("signup: %w", Validate("", 1))
	if got := Describe(wrappedV); got != "invalid name (must not be empty)" {
		t.Errorf("Describe(wrapped validation error) = %q (use errors.As)", got)
	}
}
