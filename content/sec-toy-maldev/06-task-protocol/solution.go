package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io"
)

type Task struct {
	Session  string `json:"session"`
	Sequence uint64 `json:"seq"`
	Kind     string `json:"kind"`
	Data     string `json:"data"`
}
type TaskReply struct {
	Sequence   uint64
	Kind, Data string
}

func ProcessTasks(raw []byte, session string, last uint64) ([]TaskReply, uint64, error) {
	fail := func() ([]TaskReply, uint64, error) { return nil, last, fmt.Errorf("task batch rejected") }
	if session == "" {
		return fail()
	}
	decoder := json.NewDecoder(bytes.NewReader(raw))
	decoder.DisallowUnknownFields()
	var tasks []Task
	if decoder.Decode(&tasks) != nil {
		return fail()
	}
	var extra any
	if decoder.Decode(&extra) != io.EOF {
		return fail()
	}
	if len(tasks) == 0 || len(tasks) > 16 {
		return fail()
	}
	out := []TaskReply{}
	next := last
	for i, task := range tasks {
		if next == ^uint64(0) || task.Sequence != next+1 || task.Session != session {
			return fail()
		}
		reply := TaskReply{Sequence: task.Sequence}
		switch task.Kind {
		case "echo":
			if len(task.Data) > 255 {
				return fail()
			}
			reply.Kind = "echo"
			reply.Data = task.Data
		case "stop":
			if i != len(tasks)-1 || task.Data != "" {
				return fail()
			}
			reply.Kind = "stopped"
		default:
			return fail()
		}
		out = append(out, reply)
		next = task.Sequence
	}
	return out, next, nil
}

func main() {
	raw := []byte(`[{"session":"lab","seq":1,"kind":"echo","data":"hi"}]`)
	fmt.Println(ProcessTasks(raw, "lab", 0))
}
