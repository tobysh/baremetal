package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"reflect"
	"strings"
	"testing"
)

func TestTaskSessionsAndReplay(t *testing.T) {
	for i := 0; i < 24; i++ {
		session := fmt.Sprint("lab-", i)
		last := uint64(i * 19)
		tasks := []Task{{session, last + 1, "echo", fmt.Sprintf("data-%d\x00", i)}, {session, last + 2, "stop", ""}}
		raw, _ := json.Marshal(tasks)
		before := append([]byte{}, raw...)
		got, next, err := ProcessTasks(raw, session, last)
		want := []TaskReply{{last + 1, "echo", tasks[0].Data}, {last + 2, "stopped", ""}}
		if err != nil || next != last+2 || !reflect.DeepEqual(got, want) || !bytes.Equal(raw, before) {
			t.Fatal(got, next, err)
		}
		replies, n, err := ProcessTasks(raw, session, next)
		if err == nil || replies != nil || n != next {
			t.Fatal("replay accepted")
		}
	}
}
func TestTaskBatchIsAtomic(t *testing.T) {
	cases := []string{`null`, `[]`, `{}`, `[{"session":"s","seq":1,"kind":"echo","data":"ok"},{"session":"s","seq":3,"kind":"echo"}]`, `[{"session":"wrong","seq":1,"kind":"echo"}]`, `[{"session":"s","seq":1,"kind":"execute"}]`, `[{"session":"s","seq":1,"kind":"stop","data":"x"}]`, `[{"session":"s","seq":1,"kind":"stop"},{"session":"s","seq":2,"kind":"echo"}]`, `[{"session":"s","seq":1,"kind":"echo","unexpected":true}]`, `[{"session":"s","seq":1,"kind":"echo"}] {}`, `[{"session":"s","seq":1,"kind":"echo"}] junk`}
	tooLong, _ := json.Marshal([]Task{{"s", 1, "echo", strings.Repeat("x", 256)}})
	cases = append(cases, string(tooLong))
	for i, raw := range cases {
		got, n, err := ProcessTasks([]byte(raw), "s", 0)
		if err == nil || got != nil || n != 0 {
			t.Fatal("invalid task accepted", i, got, n)
		}
	}
}
func TestTaskLimitsAndOverflow(t *testing.T) {
	tasks := []Task{}
	for i := 1; i <= 16; i++ {
		tasks = append(tasks, Task{"s", uint64(i), "echo", ""})
	}
	raw, _ := json.Marshal(tasks)
	got, next, err := ProcessTasks(raw, "s", 0)
	if err != nil || len(got) != 16 || next != 16 {
		t.Fatal(got, next, err)
	}
	tasks = append(tasks, Task{"s", 17, "echo", ""})
	raw, _ = json.Marshal(tasks)
	if got, n, err := ProcessTasks(raw, "s", 0); err == nil || got != nil || n != 0 {
		t.Fatal("batch limit")
	}
	raw, _ = json.Marshal([]Task{{"s", 0, "echo", ""}})
	last := ^uint64(0)
	if got, n, err := ProcessTasks(raw, "s", last); err == nil || got != nil || n != last {
		t.Fatal("sequence wrapped")
	}
	raw, _ = json.Marshal([]Task{{"", 1, "echo", ""}})
	if _, _, err := ProcessTasks(raw, "", 0); err == nil {
		t.Fatal("empty session")
	}
}
