package main

import (
	"fmt"
	"time"
)

func TrySend(ch chan<- int, v int) bool {
	select {
	case ch <- v:
		return true
	default:
		return false
	}
}

func TryRecv(ch <-chan int) (int, bool) {
	select {
	case v, ok := <-ch:
		return v, ok
	default:
		return 0, false
	}
}

func RecvTimeout(ch <-chan int, d time.Duration) (int, bool) {
	select {
	case v := <-ch:
		return v, true
	case <-time.After(d):
		return 0, false
	}
}

func FirstOf(a, b <-chan string) string {
	select {
	case s := <-a:
		return s
	case s := <-b:
		return s
	}
}

func main() {
	ch := make(chan int, 1)
	fmt.Println(TrySend(ch, 1), TrySend(ch, 2))
}
