For `sum_chunks`, build a `Vec` of handles with `.map(|c| thread::spawn(move || c.iter().sum::<u64>())).collect()`, and only then join them all with a second `.map(|h| h.join().unwrap())`.
===
`thread::spawn(f).join()` is a `Result`. `match` it: `Ok(v) => Ok(v)`, `Err(_) => Err(String::from("worker panicked"))`. A function pointer such as `f` can be passed straight to `spawn`.
===
For `on_new_thread`, ask for the id inside the closure: `thread::spawn(move || (f(), thread::current().id())).join().unwrap()`.
