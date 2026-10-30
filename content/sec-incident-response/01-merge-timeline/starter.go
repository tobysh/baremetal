package main

// Source is one log with a clock skew estimate (assumption, in seconds ahead of UTC).
type Source struct {
	Name        string
	SkewSeconds int64
	Lines       []string
}

// Event is one record on the common UTC clock.
type Event struct {
	Unix    int64
	Source  string
	Message string
	Line    int
}

func BuildTimeline(srcs []Source) ([]Event, []string) {
	return nil, nil
}

func main() {}
