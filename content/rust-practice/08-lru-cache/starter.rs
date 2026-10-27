use std::collections::HashMap;

struct LruCache {
    capacity: usize,
    map: HashMap<String, i32>,
    order: Vec<String>,
}

impl LruCache {
    fn new(capacity: usize) -> LruCache {
        LruCache { capacity, map: HashMap::new(), order: Vec::new() }
    }

    fn get(&mut self, key: &str) -> Option<i32> {
        None
    }

    fn put(&mut self, key: String, value: i32) -> Option<String> {
        None
    }

    fn contains(&self, key: &str) -> bool {
        false
    }

    fn len(&self) -> usize {
        0
    }

    fn keys_oldest_first(&self) -> Vec<String> {
        Vec::new()
    }
}

// Try it with Run.
fn main() {
    let mut c = LruCache::new(2);
    c.put("a".to_string(), 1);
    c.put("b".to_string(), 2);
    c.get("a");
    println!("{:?}", c.put("c".to_string(), 3));
    println!("{:?}", c.keys_oldest_first());
}
