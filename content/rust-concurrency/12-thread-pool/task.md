Implement `ThreadPool` in the starter:

- `ThreadPool::new(size: usize) -> ThreadPool` starts `size` worker threads. It must `assert!(size > 0)`.
- `execute<F>(&self, f: F) where F: FnOnce() + Send + 'static` queues a job for any idle worker. It must return without waiting for the job.
- `size(&self) -> usize` is the number of workers.
- Dropping the pool must close the queue, let workers finish every job already queued, and join them all. After `drop(pool)` returns, all submitted jobs have run.

Jobs must run on the worker threads, never on the thread that called `execute`.
