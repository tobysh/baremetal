Replace `Rc<RefCell<usize>>` with `Arc<Mutex<usize>>` and `Rc<RefCell<HashSet<ThreadId>>>` with `Arc<Mutex<HashSet<ThreadId>>>`. Clone the `Arc`s before each `thread::spawn(move || ...)`.
===
Inside each thread: `*total.lock().unwrap() += 1; ids.lock().unwrap().insert(thread::current().id());`. After joining all handles, return `(*total.lock().unwrap(), ids.lock().unwrap().len())`.
===
`round_trip`: `thread::spawn(move || (value, thread::current().id())).join().unwrap()`. The `T: Send + 'static` bound is exactly what `spawn` needs.
