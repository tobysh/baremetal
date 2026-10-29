package main

import (
	"context"
	"errors"
	"fmt"
	"reflect"
	"sync/atomic"
	"testing"
	"time"
)

type job = func(ctx context.Context) (string, error)

func within(t *testing.T, d time.Duration, what string, fn func()) {
	t.Helper()
	done := make(chan struct{})
	go func() { fn(); close(done) }()
	select {
	case <-done:
	case <-time.After(d):
		t.Fatalf("%s did not finish within %v", what, d)
	}
}

func TestRunJobs_results_in_order(t *testing.T) {
	jobs := make([]job, 12)
	want := make([]string, 12)
	for i := range jobs {
		want[i] = fmt.Sprintf("job-%d", i)
		jobs[i] = func(ctx context.Context) (string, error) {
			time.Sleep(time.Duration(12-i) * 5 * time.Millisecond) // later jobs finish first
			return fmt.Sprintf("job-%d", i), nil
		}
	}
	var got []string
	var err error
	within(t, 5*time.Second, "RunJobs", func() { got, err = RunJobs(context.Background(), 4, jobs) })
	if err != nil {
		t.Fatalf("RunJobs error = %v; want nil", err)
	}
	if !reflect.DeepEqual(got, want) {
		t.Errorf("results = %v; want them in job order %v", got, want)
	}
}

func TestRunJobs_respects_limit(t *testing.T) {
	var running, peak int32
	jobs := make([]job, 16)
	for i := range jobs {
		jobs[i] = func(ctx context.Context) (string, error) {
			cur := atomic.AddInt32(&running, 1)
			for {
				p := atomic.LoadInt32(&peak)
				if cur <= p || atomic.CompareAndSwapInt32(&peak, p, cur) {
					break
				}
			}
			time.Sleep(25 * time.Millisecond)
			atomic.AddInt32(&running, -1)
			return "x", nil
		}
	}
	within(t, 6*time.Second, "RunJobs", func() { RunJobs(context.Background(), 3, jobs) })
	if p := atomic.LoadInt32(&peak); p > 3 {
		t.Errorf("%d jobs ran at once; the limit is 3", p)
	} else if p < 2 {
		t.Errorf("peak concurrency was %d; with a limit of 3 jobs should overlap", p)
	}
}

func TestRunJobs_first_error_stops_later_jobs(t *testing.T) {
	boom := errors.New("boom")
	var ran int32
	jobs := make([]job, 6)
	jobs[0] = func(ctx context.Context) (string, error) {
		atomic.AddInt32(&ran, 1)
		return "", boom
	}
	for i := 1; i < len(jobs); i++ {
		jobs[i] = func(ctx context.Context) (string, error) {
			atomic.AddInt32(&ran, 1)
			return "late", nil
		}
	}
	var got []string
	var err error
	within(t, 4*time.Second, "RunJobs", func() { got, err = RunJobs(context.Background(), 1, jobs) })
	if !errors.Is(err, boom) {
		t.Errorf("error = %v; want boom", err)
	}
	if n := atomic.LoadInt32(&ran); n != 1 {
		t.Errorf("%d jobs ran; with limit 1 and the first job failing, no later job should start", n)
	}
	if len(got) != 6 {
		t.Errorf("result slice has length %d; want 6 (one slot per job)", len(got))
	}
	for i, s := range got {
		if s != "" {
			t.Errorf("slot %d = %q; failed or skipped jobs leave an empty string", i, s)
		}
	}
}

func TestRunJobs_error_cancels_running_jobs(t *testing.T) {
	boom := errors.New("boom")
	var cancelled int32
	started := make(chan struct{}, 2)
	waiter := func(ctx context.Context) (string, error) {
		started <- struct{}{}
		select {
		case <-ctx.Done():
			atomic.AddInt32(&cancelled, 1)
			return "", ctx.Err()
		case <-time.After(10 * time.Second):
			return "slow", nil
		}
	}
	jobs := []job{
		waiter, waiter,
		func(ctx context.Context) (string, error) {
			<-started
			<-started
			return "", boom
		},
	}
	var err error
	start := time.Now()
	within(t, 6*time.Second, "RunJobs", func() { _, err = RunJobs(context.Background(), 3, jobs) })
	if !errors.Is(err, boom) {
		t.Errorf("error = %v; want the first error (boom), not context.Canceled", err)
	}
	if n := atomic.LoadInt32(&cancelled); n != 2 {
		t.Errorf("%d running jobs saw their context cancelled; want 2", n)
	}
	if el := time.Since(start); el > 3*time.Second {
		t.Errorf("RunJobs took %v; the failure should cancel the others quickly", el)
	}
}

func TestRunJobs_caller_cancelled(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	cancel()
	var ran int32
	jobs := []job{
		func(ctx context.Context) (string, error) { atomic.AddInt32(&ran, 1); return "a", nil },
		func(ctx context.Context) (string, error) { atomic.AddInt32(&ran, 1); return "b", nil },
	}
	var err error
	within(t, 3*time.Second, "RunJobs", func() { _, err = RunJobs(ctx, 2, jobs) })
	if !errors.Is(err, context.Canceled) {
		t.Errorf("error = %v; want context.Canceled", err)
	}
	if n := atomic.LoadInt32(&ran); n != 0 {
		t.Errorf("%d jobs ran with an already-cancelled context; want 0", n)
	}
}

func TestRunJobs_edge_cases(t *testing.T) {
	var got []string
	var err error
	within(t, 2*time.Second, "RunJobs(no jobs)", func() { got, err = RunJobs(context.Background(), 3, nil) })
	if err != nil || got == nil || len(got) != 0 {
		t.Errorf("RunJobs with no jobs = %#v, %v; want an empty non-nil slice and nil", got, err)
	}
	one := []job{func(ctx context.Context) (string, error) { return "only", nil }}
	within(t, 2*time.Second, "RunJobs(limit 0)", func() { got, err = RunJobs(context.Background(), 0, one) })
	if err != nil || !reflect.DeepEqual(got, []string{"only"}) {
		t.Errorf("RunJobs with limit 0 = %v, %v; want [only], nil", got, err)
	}
}
