package main

import "fmt"

type Host struct{ Files, Keys, Tasks map[string]string }
type Step struct{ Kind, Name, Value string }
type Entry struct {
	Kind, Name string
	Existed    bool
	Old        string
}
type Journal struct{ Entries []Entry }

func Apply(h *Host, j *Journal, steps []Step, crashAt int) (bool, error) {
	return false, nil
}
func Recover(h *Host, j *Journal) error {
	return nil
}
func Audit(h, baseline *Host) []string {
	return nil
}

func main() {
	h := &Host{Files: map[string]string{"notes.txt": "original"}}
	base := &Host{Files: map[string]string{"notes.txt": "original"}}
	j := &Journal{}
	crashed, err := Apply(h, j, []Step{{"file", "notes.txt", "changed"}, {"task", "tidy", "daily"}}, 3)
	fmt.Println(crashed, err, Audit(h, base))
	fmt.Println(Recover(h, j), Audit(h, base))
}
