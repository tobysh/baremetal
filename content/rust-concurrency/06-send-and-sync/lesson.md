---
title: Send and Sync
summary: What the Send and Sync marker traits promise, why Rc and RefCell are refused by thread::spawn, and how to turn single-threaded code into thread-safe code.
minutes: 25
difficulty: medium
tags: threads, send, sync, traits
id: rust-concurrency/send-and-sync
mode: harness
---
Rust has no special "thread-safety checker". Two ordinary **marker traits** in the standard library carry all the weight, and the compiler implements them automatically for your types when every field qualifies.

- **`Send`**: a value of this type can be **moved** to another thread.
- **`Sync`**: a shared reference `&T` can be sent to another thread. Formally, `T: Sync` exactly when `&T: Send`.

`thread::spawn` is declared roughly as `fn spawn<F: FnOnce() -> T + Send + 'static, T: Send + 'static>(f: F)`. The closure, and everything it captured, must be `Send`. That one bound is what rejects the code below.

## The classic compile errors

```rust
let counter = Rc::new(RefCell::new(0));
let c = Rc::clone(&counter);
thread::spawn(move || *c.borrow_mut() += 1);
```

```text
error[E0277]: `Rc<RefCell<i32>>` cannot be sent between threads safely
  = help: the trait `Send` is not implemented for `Rc<RefCell<i32>>`
```

- **`Rc<T>` is not `Send`** (nor `Sync`): its reference count is a plain integer, and two threads cloning at once would corrupt it. Use `Arc<T>`.
- **`RefCell<T>` is `Send` but not `Sync`**: its borrow flag is not atomic, so two threads could both think they hold the only `&mut`. Use `Mutex<T>` (or `RwLock<T>`).
- **`Cell<T>`** is the same story. **Raw pointers** are neither `Send` nor `Sync`.
- **`MutexGuard<T>` is not `Send`**: some platforms require the thread that locked to be the one that unlocks.

A handy translation table:

| single-threaded | thread-safe |
| --- | --- |
| `Rc<T>` | `Arc<T>` |
| `RefCell<T>` | `Mutex<T>` / `RwLock<T>` |
| `Cell<u32>` | `AtomicU32` |

`Arc<T>` is `Send + Sync` only if `T` is `Send + Sync`. `Mutex<T>` is `Sync` when `T` is merely `Send`, because the lock makes access exclusive. That is why `Arc<Mutex<Vec<String>>>` works.

## Generic code states its needs

A function that moves a value to another thread must say so in its bounds, or the compiler complains inside the function body:

```rust
fn work_on_thread<T: Send + 'static>(value: T) -> T {
    thread::spawn(move || value).join().unwrap()
}
```

`'static` means the type holds no short-lived borrows (it does not mean the value lives forever).

## `unsafe impl`

You can promise `Send`/`Sync` yourself with `unsafe impl Send for MyType {}`. This is for wrappers around raw pointers or FFI handles, and **you** are then responsible for the guarantee. Nothing in this course needs it.

## This lesson's exercise

The starter compiles only because it never leaves one thread. `tally_on_threads` uses `Rc<RefCell<..>>`; rewrite it with `Arc` and `Mutex` and real threads (swapping the types is exactly the compiler error you would have met). Then write the generic `round_trip`.
