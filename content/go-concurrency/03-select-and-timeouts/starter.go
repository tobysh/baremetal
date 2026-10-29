package main

import (
	"fmt"
	"time"
)

func TrySend(ch chan<- int, v int) bool {
	return false
}

func TryRecv(ch <-chan int) (int, bool) {
	return 0, false
}

func RecvTimeout(ch <-chan int, d time.Duration) (int, bool) {
	return 0, false
}

func FirstOf(a, b <-chan string) string {
	return ""
}

func main() {
	ch := make(chan int, 1)
	fmt.Println(TrySend(ch, 1), TrySend(ch, 2))
}
