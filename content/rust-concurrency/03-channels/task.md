Complete these in the starter:

- `fn fan_in(producers: usize, per: usize) -> Vec<usize>`: start `producers` threads. Producer `p` sends the values `p * 1000 + k` for `k` in `0..per`. Collect everything on the calling side and return it **sorted**. The loop must end by itself when the producers finish.
- `fn pipeline(input: Vec<i32>) -> Vec<i32>`: start one worker thread that receives numbers over a channel and sends back `n * 2 + 1` over a second channel. Feed it `input`, close the input, and return the replies in order.
- `fn drain_ready(rx: &Receiver<i32>) -> Vec<i32>`: return everything currently waiting in `rx` without ever blocking.
