package main

import "fmt"
import "errors"

type Record struct{ Block, Value int }
type Journal struct {
	Committed bool
	Records   []Record
}
type Disk struct {
	Home             []int
	Journal          Journal
	FailAt, Attempts int
}

var ErrJournal = errors.New("invalid journal")
var ErrCrash = errors.New("crash")

func (d *Disk) Write(b, v int) error {
	at := d.Attempts
	d.Attempts++
	if d.FailAt >= 0 && at == d.FailAt {
		return ErrCrash
	}
	d.Home[b] = v
	return nil
}

func Recover(d *Disk) error { return nil }
func main() {
	d := &Disk{Home: []int{1, 2}, Journal: Journal{true, []Record{{0, 9}}}, FailAt: -1}
	fmt.Println(Recover(d), d.Home)
}
