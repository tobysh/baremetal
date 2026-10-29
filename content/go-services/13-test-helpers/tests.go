package main

import (
	"errors"
	"fmt"
	"os"
	"runtime"
	"strings"
	"testing"
)

// fakeTB records what a helper does. It embeds testing.TB so it satisfies the interface,
// and overrides the methods a helper may reasonably call.
type fakeTB struct {
	testing.TB
	helper   int
	errs     []string
	fatal    string
	cleanups []func()
	dirs     []string
}

func (f *fakeTB) Helper()                        { f.helper++ }
func (f *fakeTB) Name() string                   { return "fake" }
func (f *fakeTB) Errorf(format string, a ...any) { f.errs = append(f.errs, fmt.Sprintf(format, a...)) }
func (f *fakeTB) Error(a ...any)                 { f.errs = append(f.errs, fmt.Sprint(a...)) }
func (f *fakeTB) Logf(format string, a ...any)   {}
func (f *fakeTB) Log(a ...any)                   {}
func (f *fakeTB) Fail()                          { f.errs = append(f.errs, "Fail()") }
func (f *fakeTB) Cleanup(fn func())              { f.cleanups = append(f.cleanups, fn) }
func (f *fakeTB) Fatalf(format string, a ...any) {
	f.fatal = fmt.Sprintf(format, a...)
	runtime.Goexit()
}
func (f *fakeTB) Fatal(a ...any) { f.fatal = fmt.Sprint(a...); runtime.Goexit() }
func (f *fakeTB) FailNow()       { f.fatal = "FailNow()"; runtime.Goexit() }
func (f *fakeTB) TempDir() string {
	d, err := os.MkdirTemp("", "fake")
	if err != nil {
		panic(err)
	}
	f.dirs = append(f.dirs, d)
	return d
}

// run executes fn(f) in its own goroutine, so a Fatalf's Goexit only ends that goroutine.
func (f *fakeTB) run(fn func(testing.TB)) {
	done := make(chan struct{})
	go func() {
		defer close(done)
		fn(f)
	}()
	<-done
}

func (f *fakeTB) finish() {
	for i := len(f.cleanups) - 1; i >= 0; i-- {
		f.cleanups[i]()
	}
	for _, d := range f.dirs {
		os.RemoveAll(d)
	}
}

func TestAssertEqual(t *testing.T) {
	f := &fakeTB{}
	f.run(func(tb testing.TB) { AssertEqual(tb, 3, 3); AssertEqual(tb, "a", "a"); AssertEqual(tb, true, true) })
	if len(f.errs) != 0 || f.fatal != "" {
		t.Errorf("equal values reported failures: %v %q", f.errs, f.fatal)
	}

	f = &fakeTB{}
	f.run(func(tb testing.TB) { AssertEqual(tb, 3, 4) })
	if len(f.errs) != 1 || f.errs[0] != "got 3, want 4" {
		t.Errorf("AssertEqual(3, 4) reported %q; want exactly [\"got 3, want 4\"]", f.errs)
	}
	if f.fatal != "" {
		t.Error("AssertEqual must use Errorf, not Fatalf, so the test can go on")
	}
	if f.helper == 0 {
		t.Error("AssertEqual did not call t.Helper()")
	}

	f = &fakeTB{}
	f.run(func(tb testing.TB) { AssertEqual(tb, "x", "y") })
	if len(f.errs) != 1 || f.errs[0] != "got x, want y" {
		t.Errorf("AssertEqual(x, y) reported %q", f.errs)
	}
}

func TestAssertNoError(t *testing.T) {
	f := &fakeTB{}
	f.run(func(tb testing.TB) { AssertNoError(tb, nil) })
	if len(f.errs) != 0 || f.fatal != "" {
		t.Errorf("nil error reported %v %q", f.errs, f.fatal)
	}

	f = &fakeTB{}
	reachedAfter := false
	f.run(func(tb testing.TB) {
		AssertNoError(tb, errors.New("disk full"))
		reachedAfter = true
	})
	if f.fatal != "unexpected error: disk full" {
		t.Errorf("fatal message = %q; want \"unexpected error: disk full\"", f.fatal)
	}
	if reachedAfter {
		t.Error("the test went on after AssertNoError failed; use Fatalf")
	}
	if f.helper == 0 {
		t.Error("AssertNoError did not call t.Helper()")
	}
}

func TestAssertErrorIs(t *testing.T) {
	base := errors.New("base")
	f := &fakeTB{}
	f.run(func(tb testing.TB) { AssertErrorIs(tb, fmt.Errorf("ctx: %w", base), base) })
	if len(f.errs) != 0 || f.fatal != "" {
		t.Errorf("a wrapped match reported %v %q", f.errs, f.fatal)
	}

	other := errors.New("other")
	f = &fakeTB{}
	f.run(func(tb testing.TB) { AssertErrorIs(tb, other, base) })
	if len(f.errs) != 1 || f.errs[0] != "got error other, want base" {
		t.Errorf("mismatch reported %q", f.errs)
	}
	if f.helper == 0 {
		t.Error("AssertErrorIs did not call t.Helper()")
	}

	f = &fakeTB{}
	f.run(func(tb testing.TB) { AssertErrorIs(tb, nil, base) })
	if len(f.errs) != 1 || !strings.Contains(f.errs[0], "<nil>") {
		t.Errorf("a nil error must fail and print <nil>; reported %q", f.errs)
	}
}

func TestWithEnv_sets_then_restores_old_value(t *testing.T) {
	const key = "BM_TEST_HELPER_VAR"
	os.Setenv(key, "before")
	defer os.Unsetenv(key)
	f := &fakeTB{}
	f.run(func(tb testing.TB) { WithEnv(tb, key, "during") })
	if got := os.Getenv(key); got != "during" {
		t.Errorf("during the test %s = %q; want during", key, got)
	}
	if len(f.cleanups) == 0 {
		t.Fatal("WithEnv registered no cleanup; restore the variable with t.Cleanup")
	}
	f.finish()
	if got := os.Getenv(key); got != "before" {
		t.Errorf("after cleanup %s = %q; want before", key, got)
	}
}

func TestWithEnv_unsets_a_variable_that_did_not_exist(t *testing.T) {
	const key = "BM_TEST_HELPER_FRESH"
	os.Unsetenv(key)
	f := &fakeTB{}
	f.run(func(tb testing.TB) { WithEnv(tb, key, "temp") })
	if os.Getenv(key) != "temp" {
		t.Error("variable not set")
	}
	f.finish()
	if v, ok := os.LookupEnv(key); ok {
		t.Errorf("variable still set to %q after cleanup; it should be unset again", v)
	}
}

func TestWithEnv_distinguishes_empty_from_unset(t *testing.T) {
	const key = "BM_TEST_HELPER_EMPTY"
	os.Setenv(key, "")
	defer os.Unsetenv(key)
	f := &fakeTB{}
	f.run(func(tb testing.TB) { WithEnv(tb, key, "x") })
	f.finish()
	if v, ok := os.LookupEnv(key); !ok || v != "" {
		t.Errorf("after cleanup: %q, set=%v; want empty but still set (use os.LookupEnv)", v, ok)
	}
}

func TestWithEnv_nested_cleanups_run_in_reverse(t *testing.T) {
	const key = "BM_TEST_HELPER_NEST"
	os.Setenv(key, "orig")
	defer os.Unsetenv(key)
	f := &fakeTB{}
	f.run(func(tb testing.TB) { WithEnv(tb, key, "one"); WithEnv(tb, key, "two") })
	f.finish()
	if got := os.Getenv(key); got != "orig" {
		t.Errorf("after both cleanups %s = %q; want orig", key, got)
	}
}

func TestTempFile(t *testing.T) {
	f := &fakeTB{}
	var p1, p2 string
	f.run(func(tb testing.TB) {
		p1 = TempFile(tb, "a.txt", "hello")
		p2 = TempFile(tb, "b.txt", "")
	})
	defer f.finish()
	if f.fatal != "" || len(f.errs) != 0 {
		t.Fatalf("TempFile failed: %v %q", f.errs, f.fatal)
	}
	if len(f.dirs) == 0 {
		t.Error("TempFile did not use t.TempDir()")
	}
	if b, err := os.ReadFile(p1); err != nil || string(b) != "hello" {
		t.Errorf("file 1 = %q, %v", b, err)
	}
	if b, err := os.ReadFile(p2); err != nil || string(b) != "" {
		t.Errorf("file 2 = %q, %v", b, err)
	}
	if !strings.HasSuffix(p1, "a.txt") || !strings.HasSuffix(p2, "b.txt") {
		t.Errorf("paths %q %q should end with the file names", p1, p2)
	}
	if f.helper == 0 {
		t.Error("TempFile did not call t.Helper()")
	}
}

func TestTempFile_write_error_is_fatal(t *testing.T) {
	f := &fakeTB{}
	f.run(func(tb testing.TB) { TempFile(tb, "no-such-dir/x.txt", "data") })
	defer f.finish()
	if f.fatal == "" {
		t.Error("a failed write should end the test with Fatalf")
	}
}
