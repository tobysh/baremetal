package main

import (
	"reflect"
	"testing"
	"time"
)

func TestScheduleRetriesWithBackoff(t *testing.T) {
	got, err := Schedule([]Job{{ID: "a", Start: 0, Fails: 2, MaxTries: 3, Backoff: 5}}, 1000)
	want := []Event{{0, "a", "retry"}, {5, "a", "retry"}, {15, "a", "done"}}
	if err != nil || !reflect.DeepEqual(got, want) {
		t.Fatal(got, err)
	}
	got, err = Schedule([]Job{{ID: "z", Start: 1, Fails: 5, MaxTries: 2, Backoff: 3}}, 1000)
	want = []Event{{1, "z", "retry"}, {4, "z", "gave-up"}}
	if err != nil || !reflect.DeepEqual(got, want) {
		t.Fatal(got, err)
	}
	got, _ = Schedule([]Job{{ID: "q", Start: 7, MaxTries: 1}}, 7)
	if !reflect.DeepEqual(got, []Event{{7, "q", "done"}}) {
		t.Fatal("attempt exactly at the horizon must run", got)
	}
}

func TestScheduleCancellation(t *testing.T) {
	got, _ := Schedule([]Job{{ID: "c", Start: 0, Fails: 9, MaxTries: 8, Backoff: 4, CancelAt: 10}}, 1000)
	want := []Event{{0, "c", "retry"}, {4, "c", "retry"}, {10, "c", "cancelled"}}
	if !reflect.DeepEqual(got, want) {
		t.Fatal("cancellation must happen at its own tick", got)
	}
	got, _ = Schedule([]Job{{ID: "c", Start: 3, MaxTries: 1, CancelAt: 3}}, 1000)
	if !reflect.DeepEqual(got, []Event{{3, "c", "cancelled"}}) {
		t.Fatal("cancel beats a simultaneous attempt", got)
	}
	got, _ = Schedule([]Job{{ID: "c", Start: 3, MaxTries: 1, CancelAt: 4}}, 1000)
	if !reflect.DeepEqual(got, []Event{{3, "c", "done"}}) {
		t.Fatal("cancel after completion is irrelevant", got)
	}
	got, _ = Schedule([]Job{{ID: "c", Start: 0, Fails: 3, MaxTries: 4, Backoff: 2, CancelAt: 500}}, 20)
	if len(got) == 0 || !reflect.DeepEqual(got[len(got)-1], Event{14, "c", "done"}) {
		t.Fatal("cancel beyond the horizon", got)
	}
}

func TestScheduleHorizonAndInterleaving(t *testing.T) {
	got, _ := Schedule([]Job{{ID: "a", Start: 0, Fails: 3, MaxTries: 4, Backoff: 10}, {ID: "b", Start: 99, MaxTries: 1}}, 25)
	want := []Event{{0, "a", "retry"}, {10, "a", "retry"}, {25, "a", "horizon"}, {25, "b", "horizon"}}
	if !reflect.DeepEqual(got, want) {
		t.Fatal(got)
	}
	jobs := []Job{{ID: "a", Start: 0, Fails: 1, MaxTries: 2, Backoff: 7}, {ID: "b", Start: 3, MaxTries: 1}}
	got, _ = Schedule(jobs, 100)
	want = []Event{{0, "a", "retry"}, {3, "b", "done"}, {7, "a", "done"}}
	if !reflect.DeepEqual(got, want) {
		t.Fatal(got)
	}
}

func TestScheduleOrderIsIndependentOfInputOrder(t *testing.T) {
	jobs := []Job{
		{ID: "m", Start: 5, Fails: 1, MaxTries: 3, Backoff: 5},
		{ID: "b", Start: 5, MaxTries: 1},
		{ID: "x", Start: 5, Fails: 4, MaxTries: 2, Backoff: 5},
		{ID: "a", Start: 10, MaxTries: 1, CancelAt: 10},
	}
	want, err := Schedule(jobs, 100)
	if err != nil || len(want) < 3 || want[0] != (Event{5, "b", "done"}) || want[1] != (Event{5, "m", "retry"}) || want[2] != (Event{5, "x", "retry"}) {
		t.Fatal(want, err)
	}
	before := append([]Job{}, jobs...)
	orders := [][]int{{3, 2, 1, 0}, {1, 3, 0, 2}, {2, 0, 3, 1}}
	for _, o := range orders {
		shuffled := make([]Job, 0, 4)
		for _, i := range o {
			shuffled = append(shuffled, jobs[i])
		}
		got, _ := Schedule(shuffled, 100)
		if !reflect.DeepEqual(got, want) {
			t.Fatal("order depends on input order", got)
		}
	}
	if !reflect.DeepEqual(jobs, before) {
		t.Fatal("input modified")
	}
}

func TestScheduleIsEventDrivenNotTickDriven(t *testing.T) {
	start := time.Now()
	got, err := Schedule([]Job{{ID: "slow", Start: 1 << 31, Fails: 2, MaxTries: 3, Backoff: 1 << 20}}, 1<<32)
	if err != nil || len(got) != 3 || got[2].At != 1<<31+3<<20 {
		t.Fatal(got, err)
	}
	if time.Since(start) > 500*time.Millisecond {
		t.Fatal("scheduler is stepping tick by tick")
	}
}

func TestScheduleRejectsInvalidInput(t *testing.T) {
	ok := Job{ID: "a", MaxTries: 1}
	cases := []struct {
		jobs    []Job
		horizon uint64
	}{
		{nil, 10},
		{[]Job{ok, ok}, 10},
		{[]Job{{ID: "", MaxTries: 1}}, 10},
		{[]Job{{ID: "0123456789012345678901234567890123", MaxTries: 1}}, 10},
		{[]Job{{ID: "a", MaxTries: 0}}, 10},
		{[]Job{{ID: "a", MaxTries: 9, Backoff: 1}}, 10},
		{[]Job{{ID: "a", MaxTries: 2, Backoff: 0}}, 10},
		{[]Job{{ID: "a", MaxTries: 2, Backoff: 1<<20 + 1}}, 10},
		{[]Job{{ID: "a", MaxTries: 1, Fails: -1}}, 10},
		{[]Job{{ID: "a", MaxTries: 1, Fails: 17}}, 10},
		{[]Job{{ID: "a", MaxTries: 1, Start: 1<<32 + 1}}, 10},
		{[]Job{{ID: "a", MaxTries: 1, CancelAt: 1<<32 + 1}}, 10},
		{[]Job{ok}, 1<<32 + 1},
	}
	many := make([]Job, 33)
	for i := range many {
		many[i] = Job{ID: string(rune('A' + i)), MaxTries: 1}
	}
	cases = append(cases, struct {
		jobs    []Job
		horizon uint64
	}{many, 10})
	for i, c := range cases {
		got, err := Schedule(c.jobs, c.horizon)
		if err == nil || got != nil {
			t.Fatal("invalid input accepted", i)
		}
	}
}
