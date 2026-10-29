Use threads and `std::sync::mpsc` channels (not shared locks).

- `fn fan_out_sum(chunks: Vec<Vec<u64>>) -> u64`: sum each chunk on its own thread, send the partial sums over a channel, and return the total of all chunks. No chunks gives 0.
- `fn squares_in_order(inputs: Vec<u32>) -> Vec<u64>`: one thread per input computes its square (as `u64`) and sends it with the input's index; return the squares in input order.
- `fn collect_from_producers(producers: usize, per: usize) -> Vec<Vec<usize>>`: start `producers` threads; producer `p` sends the numbers `0, 1, ..., per - 1` one at a time, each as a `(p, k)` message. The receiver returns, for each producer index, the list of `k` values **in the order they arrived**.
- `fn pipeline(input: Vec<i32>) -> Vec<i32>`: three stages on three threads connected by channels: the first doubles each number, the second adds 1, and the main thread collects the results in order. Join the stage threads before returning.
