In `fan_in`, give each producer `let tx = tx.clone();` and `move` it in. After the spawn loop call `drop(tx)`, otherwise `for v in rx` waits forever. Sort the collected `Vec` at the end.
===
For `pipeline` you need two channels: `(in_tx, in_rx)` for the input and `(out_tx, out_rx)` for the replies. The worker does `for n in in_rx { out_tx.send(n * 2 + 1).unwrap(); }`. Send everything, `drop(in_tx)`, then `out_rx.iter().collect()`. The worker's `out_tx` is dropped when its loop ends.
===
`rx.try_iter().collect()` is the whole of `drain_ready`.
