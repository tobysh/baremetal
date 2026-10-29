- `Merge(chans ...<-chan int) <-chan int` forwards every value from every input into one output channel, which it closes once **all** inputs are closed and drained. Values from the same input keep their order. With no inputs, the output is closed immediately. Return the channel right away.
- `FanOut(in <-chan int, workers int, f func(int) int) []<-chan int` starts `workers` goroutines (treat `workers < 1` as `1`) that all read from `in`. Each worker sends `f(v)` for the values it receives to **its own** output channel, and closes that channel when `in` is closed. Return the slice of output channels (one per worker) right away.

Together, `Merge(FanOut(in, 4, f)...)` is a parallel map whose results arrive in any order.
