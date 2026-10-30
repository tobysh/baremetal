package main

import (
	"fmt"
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
	return nil, last, nil
}

func main() {
	raw := []byte(`[{"session":"lab","seq":1,"kind":"echo","data":"hi"}]`)
	fmt.Println(ProcessTasks(raw, "lab", 0))
}
