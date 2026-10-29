use std::collections::HashMap;
use std::sync::{Arc, RwLock};

struct Registry {
    inner: Arc<RwLock<HashMap<String, u32>>>,
}

impl Registry {
    fn new() -> Registry {
        Registry { inner: Arc::new(RwLock::new(HashMap::new())) }
    }

    fn handle(&self) -> Registry {
        Registry { inner: Arc::clone(&self.inner) }
    }

    fn get(&self, key: &str) -> Option<u32> {
        self.inner.read().unwrap().get(key).copied()
    }

    fn set(&self, key: &str, value: u32) {
        self.inner.write().unwrap().insert(key.to_string(), value);
    }

    fn incr(&self, key: &str) -> u32 {
        let mut m = self.inner.write().unwrap();
        let v = m.entry(key.to_string()).or_insert(0);
        *v += 1;
        *v
    }

    fn snapshot(&self) -> Vec<(String, u32)> {
        let m = self.inner.read().unwrap();
        let mut out: Vec<(String, u32)> = m.iter().map(|(k, v)| (k.clone(), *v)).collect();
        out.sort();
        out
    }
}

fn can_read_now(lock: &RwLock<i32>) -> bool {
    lock.try_read().is_ok()
}

// Try it with Run.
fn main() {
    let r = Registry::new();
    r.set("a", 5);
    println!("{:?} {}", r.get("a"), r.incr("a"));
}
