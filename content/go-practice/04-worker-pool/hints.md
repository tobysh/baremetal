`Process`: `out := make([]int, len(inputs))`. Make a `jobs := make(chan int)` that carries *indexes*. Each worker does `for i := range jobs { out[i] = f(inputs[i]) }`.
===
Feed the indexes in the calling goroutine (`for i := range inputs { jobs <- i }`), then `close(jobs)` and `wg.Wait()` before returning `out`.
===
`Generate`: make a channel, start a goroutine that sends each number and then `close`s it (use `defer close(out)`), and return the channel right away.
===
`Square` has the same shape as `Generate`, but loops over `for n := range in`. `Collect` is `for v := range in { result = append(result, v) }`.
