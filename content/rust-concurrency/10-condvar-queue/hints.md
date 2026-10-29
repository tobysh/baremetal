`push`: `self.items.lock().unwrap().push_back(item); self.ready.notify_one();`.
===
`pop`: `let mut g = self.items.lock().unwrap(); loop { if let Some(x) = g.pop_front() { return x; } g = self.ready.wait(g).unwrap(); }`. The loop handles spurious wakeups.
===
`pop_timeout`: `let g = self.items.lock().unwrap(); let (mut g, _) = self.ready.wait_timeout_while(g, timeout, |q| q.is_empty()).unwrap(); g.pop_front()`. If it timed out the queue is still empty, so `pop_front` gives `None`.
