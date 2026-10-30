package main

import (
	"reflect"
	"testing"
	"time"
)

// drain reads ch until it is closed, failing the test instead of hanging.
func drain(t *testing.T, ch <-chan int) []int {
	t.Helper()
	got := []int{}
	timeout := time.After(2 * time.Second)
	for {
		select {
		case v, ok := <-ch:
			if !ok {
				return got
			}
			got = append(got, v)
		case <-timeout:
			t.Fatalf("timed out waiting for the channel to be closed (did the stage forget close()?) after receiving %v", got)
			return got
		}
	}
}

func TestGenerate_sends_every_number_in_order(t *testing.T) {
	got := drain(t, Generate(4, 5, 6))
	if !reflect.DeepEqual(got, []int{4, 5, 6}) {
		t.Errorf("Generate(4, 5, 6) delivered %v; want [4 5 6]", got)
	}
}

func TestGenerate_returns_without_a_receiver(t *testing.T) {
	done := make(chan struct{})
	go func() {
		Generate(1, 2, 3)
		close(done)
	}()
	select {
	case <-done:
	case <-time.After(time.Second):
		t.Fatalf("Generate blocked with no receiver; it must start a goroutine and return immediately")
	}
}

func TestGenerate_with_no_numbers_closes_the_channel(t *testing.T) {
	got := drain(t, Generate())
	if len(got) != 0 {
		t.Errorf("Generate() delivered %v; want nothing", got)
	}
}

func TestSquare_squares_each_value(t *testing.T) {
	in := make(chan int)
	go func() {
		defer close(in)
		for _, n := range []int{1, -2, 3} {
			in <- n
		}
	}()
	got := drain(t, Square(in))
	if !reflect.DeepEqual(got, []int{1, 4, 9}) {
		t.Errorf("Square of 1, -2, 3 delivered %v; want [1 4 9]", got)
	}
}

func TestCollect_gathers_everything(t *testing.T) {
	in := make(chan int, 3)
	in <- 7
	in <- 8
	in <- 9
	close(in)
	got := Collect(in)
	if !reflect.DeepEqual(got, []int{7, 8, 9}) {
		t.Errorf("Collect = %v; want [7 8 9]", got)
	}
}

func TestCollect_empty_channel_gives_empty_slice(t *testing.T) {
	in := make(chan int)
	close(in)
	got := Collect(in)
	if got == nil || len(got) != 0 {
		t.Errorf("Collect(closed empty channel) = %#v; want []int{}", got)
	}
}

func TestPipeline_end_to_end(t *testing.T) {
	result := make(chan []int, 1)
	go func() { result <- Collect(Square(Generate(1, 2, 3, 4, 5))) }()
	select {
	case got := <-result:
		if !reflect.DeepEqual(got, []int{1, 4, 9, 16, 25}) {
			t.Errorf("Collect(Square(Generate(1..5))) = %v; want [1 4 9 16 25]", got)
		}
	case <-time.After(2 * time.Second):
		t.Fatalf("the pipeline never finished: a stage is blocked or never closes its channel")
	}
}
