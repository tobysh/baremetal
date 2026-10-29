Use fields `workers: Vec<JoinHandle<()>>` and `sender: Option<mpsc::Sender<Job>>`. In `new`: `let (tx, rx) = mpsc::channel::<Job>(); let rx = Arc::new(Mutex::new(rx));` and spawn `size` threads, each with `Arc::clone(&rx)`.
===
Worker loop: `loop { let job = rx.lock().unwrap().recv(); match job { Ok(job) => job(), Err(_) => break } }`. `execute` does `self.sender.as_ref().unwrap().send(Box::new(f)).unwrap();`.
===
`impl Drop`: `drop(self.sender.take());` then `for w in self.workers.drain(..) { w.join().unwrap(); }`. Without dropping the sender first, the workers would wait forever and `join` would hang.
