package main

// Generate sends nums on the returned channel, then closes it.
func Generate(nums ...int) <-chan int {
	out := make(chan int)
	go func() {
		defer close(out)
		for _, n := range nums {
			out <- n
		}
	}()
	return out
}

// Square returns a channel of the squares of the values read from in.
func Square(in <-chan int) <-chan int {
	out := make(chan int)
	go func() {
		defer close(out)
		for v := range in {
			out <- v * v
		}
	}()
	return out
}

// Collect gathers everything from in until it is closed.
func Collect(in <-chan int) []int {
	result := []int{}
	for v := range in {
		result = append(result, v)
	}
	return result
}
