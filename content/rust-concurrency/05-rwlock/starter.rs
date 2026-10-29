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
        Registry::new()
    }

    fn get(&self, key: &str) -> Option<u32> {
        None
    }

    fn set(&self, key: &str, value: u32) {
    }

    fn incr(&self, key: &str) -> u32 {
        0
    }

    fn snapshot(&self) -> Vec<(String, u32)> {
        Vec::new()
    }
}

fn can_read_now(lock: &RwLock<i32>) -> bool {
    false
}

// Try it with Run.
fn main() {
    let r = Registry::new();
    r.set("a", 5);
    println!("{:?} {}", r.get("a"), r.incr("a"));
}
