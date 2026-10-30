package main

import (
	"errors"
	"reflect"
	"strings"
	"testing"
)

func TestSafeDivide_normal(t *testing.T) {
	for _, c := range []struct{ a, b, want int }{{10, 2, 5}, {7, 2, 3}, {-9, 3, -3}, {0, 5, 0}} {
		got, err := SafeDivide(c.a, c.b)
		if err != nil || got != c.want {
			t.Errorf("SafeDivide(%d, %d) = %d, %v; want %d, nil", c.a, c.b, got, err, c.want)
		}
	}
}

func TestSafeDivide_zero_becomes_error(t *testing.T) {
	for _, a := range []int{1, 0, -8} {
		got, err := SafeDivide(a, 0)
		if err == nil || !strings.Contains(err.Error(), "divide by zero") {
			t.Errorf("SafeDivide(%d, 0) error = %v; want one containing \"divide by zero\"", a, err)
		}
		if got != 0 {
			t.Errorf("SafeDivide(%d, 0) result = %d; want 0", a, got)
		}
	}
}

func TestTry_no_panic(t *testing.T) {
	ran := false
	if err := Try(func() { ran = true }); err != nil || !ran {
		t.Errorf("Try with a calm function: err = %v, ran = %v; want nil, true", err, ran)
	}
}

func TestTry_error_value_is_returned_as_is(t *testing.T) {
	sentinel := errors.New("boom")
	err := Try(func() { panic(sentinel) })
	if !errors.Is(err, sentinel) {
		t.Errorf("Try(panic(sentinel)) = %v; want the same error value", err)
	}
}

func TestTry_string_and_other_values(t *testing.T) {
	if err := Try(func() { panic("oops") }); err == nil || err.Error() != "oops" {
		t.Errorf("Try(panic(\"oops\")) = %v; want error \"oops\"", err)
	}
	if err := Try(func() { panic(42) }); err == nil || err.Error() != "panic: 42" {
		t.Errorf("Try(panic(42)) = %v; want error \"panic: 42\"", err)
	}
}

func TestTry_runtime_panic(t *testing.T) {
	var m map[string]int
	err := Try(func() { m["x"] = 1 })
	if err == nil || !strings.Contains(err.Error(), "nil map") {
		t.Errorf("Try(write to nil map) = %v; want an error mentioning the nil map", err)
	}
	xs := []int{1}
	i := 5
	if err := Try(func() { _ = xs[i] }); err == nil {
		t.Errorf("Try(index out of range) = nil; want an error")
	}
}

func TestDeferOrder_is_last_in_first_out(t *testing.T) {
	if got := DeferOrder("a", "b", "c"); !reflect.DeepEqual(got, []string{"c", "b", "a"}) {
		t.Errorf("DeferOrder(a, b, c) = %v; want [c b a]", got)
	}
	if got := DeferOrder("x"); !reflect.DeepEqual(got, []string{"x"}) {
		t.Errorf("DeferOrder(x) = %v; want [x]", got)
	}
	if got := DeferOrder(); len(got) != 0 {
		t.Errorf("DeferOrder() = %v; want empty", got)
	}
}

func TestWithCleanup_normal_and_error(t *testing.T) {
	cleaned := 0
	if err := WithCleanup(func() error { return nil }, func() { cleaned++ }); err != nil || cleaned != 1 {
		t.Errorf("success: err = %v, cleanup ran %d times; want nil, 1", err, cleaned)
	}
	sentinel := errors.New("failed")
	cleaned = 0
	if err := WithCleanup(func() error { return sentinel }, func() { cleaned++ }); err != sentinel || cleaned != 1 {
		t.Errorf("failure: err = %v, cleanup ran %d times; want the work error, 1", err, cleaned)
	}
}

func TestWithCleanup_runs_cleanup_and_keeps_panicking(t *testing.T) {
	cleaned := 0
	var recovered any
	func() {
		defer func() { recovered = recover() }()
		_ = WithCleanup(func() error { panic("bad work") }, func() { cleaned++ })
	}()
	if cleaned != 1 {
		t.Errorf("cleanup ran %d times during a panic; want 1", cleaned)
	}
	if recovered != "bad work" {
		t.Errorf("the panic should still reach the caller; recovered %v", recovered)
	}
}
