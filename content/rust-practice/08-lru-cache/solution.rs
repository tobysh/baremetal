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

    fn touch(&mut self, key: &str) {
        if let Some(pos) = self.order.iter().position(|k| k == key) {
            let k = self.order.remove(pos);
            self.order.push(k);
        }
    }

    fn get(&mut self, key: &str) -> Option<i32> {
        let value = *self.map.get(key)?;
        self.touch(key);
        Some(value)
    }

    fn put(&mut self, key: String, value: i32) -> Option<String> {
        if self.capacity == 0 {
            return None;
        }
        if self.map.contains_key(&key) {
            self.map.insert(key.clone(), value);
            self.touch(&key);
            return None;
        }
        let mut evicted = None;
        if self.map.len() == self.capacity {
            let oldest = self.order.remove(0);
            self.map.remove(&oldest);
            evicted = Some(oldest);
        }
        self.map.insert(key.clone(), value);
        self.order.push(key);
        evicted
    }

    fn contains(&self, key: &str) -> bool {
        self.map.contains_key(key)
    }

    fn len(&self) -> usize {
        self.map.len()
    }

    fn keys_oldest_first(&self) -> Vec<String> {
        self.order.clone()
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
