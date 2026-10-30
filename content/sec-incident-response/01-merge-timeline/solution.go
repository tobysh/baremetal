package main

import (
	"fmt"
	"sort"
	"strings"
	"time"
)

type Source struct {
	Name        string
	SkewSeconds int64
	Lines       []string
}

type Event struct {
	Unix    int64
	Source  string
	Message string
	Line    int
}

func BuildTimeline(srcs []Source) ([]Event, []string) {
	events := []Event{}
	rejected := []string{}
	for _, s := range srcs {
		for i, l := range s.Lines {
			ts, msg, ok := strings.Cut(l, " ")
			t, err := time.Parse(time.RFC3339, ts)
			if !ok || msg == "" || err != nil {
				rejected = append(rejected, fmt.Sprintf("%s:%d", s.Name, i+1))
				continue
			}
			events = append(events, Event{t.Unix() - s.SkewSeconds, s.Name, msg, i + 1})
		}
	}
	sort.SliceStable(events, func(a, b int) bool {
		x, y := events[a], events[b]
		if x.Unix != y.Unix {
			return x.Unix < y.Unix
		}
		if x.Source != y.Source {
			return x.Source < y.Source
		}
		return x.Line < y.Line
	})
	return events, rejected
}

func main() {}
