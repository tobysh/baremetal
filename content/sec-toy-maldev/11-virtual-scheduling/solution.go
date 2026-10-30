package main

import (
	"fmt"
	"sort"
)

type Job struct {
	ID       string
	Start    uint64
	Fails    int
	MaxTries int
	Backoff  uint64
	CancelAt uint64
}
type Event struct {
	At   uint64
	ID   string
	Kind string
}

type jobState struct {
	job  Job
	at   uint64
	try  int
	live bool
}

func Schedule(jobs []Job, horizon uint64) ([]Event, error) {
	if len(jobs) == 0 || len(jobs) > 32 || horizon > 1<<32 {
		return nil, fmt.Errorf("invalid schedule")
	}
	seen := map[string]bool{}
	for _, j := range jobs {
		if j.ID == "" || len(j.ID) > 32 || seen[j.ID] || j.Start > 1<<32 || j.CancelAt > 1<<32 ||
			j.Fails < 0 || j.Fails > 16 || j.MaxTries < 1 || j.MaxTries > 8 || j.Backoff > 1<<20 ||
			(j.MaxTries > 1 && j.Backoff == 0) {
			return nil, fmt.Errorf("invalid job")
		}
		seen[j.ID] = true
	}
	states := make([]*jobState, len(jobs))
	for i, j := range jobs {
		states[i] = &jobState{job: j, at: j.Start, try: 1, live: true}
	}
	sort.Slice(states, func(a, b int) bool { return states[a].job.ID < states[b].job.ID })
	next := func(s *jobState) (uint64, bool) {
		if s.job.CancelAt != 0 && s.job.CancelAt <= s.at {
			return s.job.CancelAt, true
		}
		return s.at, false
	}
	events := []Event{}
	for {
		var best *jobState
		var bestAt uint64
		var bestCancel bool
		for _, s := range states {
			if !s.live {
				continue
			}
			at, cancel := next(s)
			if at <= horizon && (best == nil || at < bestAt) {
				best, bestAt, bestCancel = s, at, cancel
			}
		}
		if best == nil {
			break
		}
		j := best.job
		switch {
		case bestCancel:
			events = append(events, Event{bestAt, j.ID, "cancelled"})
			best.live = false
		case best.try > j.Fails:
			events = append(events, Event{bestAt, j.ID, "done"})
			best.live = false
		case best.try == j.MaxTries:
			events = append(events, Event{bestAt, j.ID, "gave-up"})
			best.live = false
		default:
			events = append(events, Event{bestAt, j.ID, "retry"})
			best.at = bestAt + j.Backoff<<(best.try-1)
			best.try++
		}
	}
	for _, s := range states {
		if s.live {
			events = append(events, Event{horizon, s.job.ID, "horizon"})
		}
	}
	return events, nil
}

func main() {
	jobs := []Job{{ID: "sync", Start: 0, Fails: 2, MaxTries: 3, Backoff: 5}, {ID: "tidy", Start: 3, MaxTries: 1}}
	events, err := Schedule(jobs, 100)
	fmt.Println(events, err)
}
