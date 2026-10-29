---
title: Barrier, Once and OnceLock
summary: Make threads wait for each other with Barrier, and run one-time initialisation exactly once with Once and OnceLock.
minutes: 25
difficulty: medium
tags: threads, barrier, once, oncelock
id: rust-concurrency/barrier-once
mode: harness
---
Two more coordination tools from `std::sync`: one for "everyone wait until we are all here", one for "do this exactly once, no matter how many threads ask".

## Barrier: rendezvous points

`Barrier::new(n)` makes a gate that opens only when `n` threads have called `wait()`. Everyone is held until the last arrives, and then all proceed together. It is reusable: the next round of `wait()` calls works again.

```rust
let barrier = Arc::new(Barrier::new(3));
for _ in 0..3 {
    let b = Arc::clone(&barrier);
    thread::spawn(move || {
        do_phase_one();
        b.wait();            // nobody starts phase two before all finish phase one
        do_phase_two();
    });
}
```

`wait()` returns a `BarrierWaitResult`; exactly **one** of the `n` threads in each round gets `is_leader() == true`, handy for "one thread prints the summary".

A barrier turns many threads into a series of lock-step phases, as in simulations where every step reads the previous step's results. If fewer than `n` threads ever arrive, the others wait forever, so count carefully.

## Once: run exactly one time

`std::sync::Once` runs a closure the first time `call_once` is invoked; every other caller (even concurrent ones) blocks until it has finished and then skips it:

```rust
static INIT: Once = Once::new();
INIT.call_once(|| setup_logging());
```

## OnceLock: a value initialised once

`OnceLock<T>` is a `static`-friendly cell that holds a value computed on first use and then hands out `&T` to everybody:

```rust
static CONFIG: OnceLock<String> = OnceLock::new();

fn config() -> &'static String {
    CONFIG.get_or_init(|| load_config())    // closure runs at most once
}
```

If several threads race on the first call, one closure runs and the others wait for its result. This replaces the old `lazy_static` crate for most needs and gives you global state without `static mut`.

## This lesson's exercise

Write `lockstep`, `leader_count`, `ensure_init` and `config`. The tests count how many times your initialisers ran.
