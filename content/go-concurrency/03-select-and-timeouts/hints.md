`TrySend`: `select { case ch <- v: return true; default: return false }`.
===
`TryRecv`: `select { case v, ok := <-ch: return v, ok; default: return 0, false }`. A closed channel gives `ok == false`, which is exactly what the task wants. When `ok` is false `v` is already `0`.
===
`RecvTimeout`: two cases, `case v := <-ch: return v, true` and `case <-time.After(d): return 0, false`.
===
`FirstOf` is a `select` with one receive case per channel.
