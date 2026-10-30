Build a three-stage pipeline:

- `Generate(nums ...int) <-chan int` sends each number on a channel from a new goroutine, then closes the channel.
- `Square(in <-chan int) <-chan int` returns a channel carrying the square of each value received from `in`; it closes its output when `in` is closed.
- `Collect(in <-chan int) []int` reads until `in` is closed and returns everything received, in order (an empty, non-nil slice if nothing was received).

Each stage must return immediately and do its work in a goroutine.
