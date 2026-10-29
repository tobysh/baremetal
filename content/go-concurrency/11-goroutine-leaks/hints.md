`First`: `ch := make(chan int, len(fns))`. Each goroutine sends its result to `ch`. Return `<-ch`. Because the buffer fits every result, the slower goroutines can always deposit theirs and exit.
===
`Generator`: inside the producer goroutine, replace the bare `out <- i` with a `select` that also has `case <-done: return`. Add `defer close(out)`.
===
Check yourself: for each `go` statement, ask what makes that goroutine stop in the worst case (the consumer vanishes).
