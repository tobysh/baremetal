package main

import "fmt"

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

func Schedule(jobs []Job, horizon uint64) ([]Event, error) {
	return nil, nil
}

func main() {
	jobs := []Job{{ID: "sync", Start: 0, Fails: 2, MaxTries: 3, Backoff: 5}, {ID: "tidy", Start: 3, MaxTries: 1}}
	events, err := Schedule(jobs, 100)
	fmt.Println(events, err)
}
