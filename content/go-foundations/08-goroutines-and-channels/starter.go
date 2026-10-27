package main

// Generate sends nums on the returned channel, then closes it.
func Generate(nums ...int) <-chan int {
	out := make(chan int)
	close(out)
	return out
}

// Square returns a channel of the squares of the values read from in.
func Square(in <-chan int) <-chan int {
	out := make(chan int)
	close(out)
	return out
}

// Collect gathers everything from in until it is closed.
func Collect(in <-chan int) []int {
	return nil
}
