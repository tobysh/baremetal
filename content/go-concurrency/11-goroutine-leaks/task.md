Both functions below must leave **no goroutines behind**.

- `First(fns ...func() int) int` runs every function in its own goroutine and returns the result of whichever finishes first, without waiting for the others. With no functions, return `0` immediately. The slower functions still run to completion, but their results must not leave goroutines stuck.
- `Generator(done <-chan struct{}) <-chan int` returns a channel that yields `0, 1, 2, ...`. When `done` is closed, the producing goroutine must exit and close the output, **even if nobody is reading it any more**.
