fn s(x: &str) -> String {
    x.to_string()
}

#[test]
fn put_and_get() {
    let mut c = LruCache::new(3);
    assert_eq!(c.put(s("a"), 1), None, "no eviction while there is room");
    assert_eq!(c.put(s("b"), 2), None, "still room");
    assert_eq!(c.get("a"), Some(1), "a is stored");
    assert_eq!(c.get("b"), Some(2), "b is stored");
    assert_eq!(c.get("zzz"), None, "missing key");
    assert_eq!(c.len(), 2, "two keys stored");
}

#[test]
fn evicts_the_oldest() {
    let mut c = LruCache::new(2);
    c.put(s("a"), 1);
    c.put(s("b"), 2);
    assert_eq!(c.put(s("c"), 3), Some(s("a")), "a is the least recently used");
    assert!(!c.contains("a"), "a is gone");
    assert!(c.contains("b") && c.contains("c"), "b and c remain");
    assert_eq!(c.get("a"), None, "a cannot be fetched");
    assert_eq!(c.len(), 2, "capacity respected");
}

#[test]
fn get_refreshes_a_key() {
    let mut c = LruCache::new(2);
    c.put(s("a"), 1);
    c.put(s("b"), 2);
    assert_eq!(c.get("a"), Some(1), "touch a");
    assert_eq!(c.put(s("c"), 3), Some(s("b")), "b is now the oldest, not a");
    assert_eq!(c.keys_oldest_first(), vec![s("a"), s("c")], "order after eviction");
}

#[test]
fn contains_does_not_refresh() {
    let mut c = LruCache::new(2);
    c.put(s("a"), 1);
    c.put(s("b"), 2);
    assert!(c.contains("a"), "a is present");
    assert_eq!(c.keys_oldest_first(), vec![s("a"), s("b")], "contains leaves the order alone");
    assert_eq!(c.put(s("c"), 3), Some(s("a")), "so a is still the oldest");
}

#[test]
fn updating_replaces_and_refreshes() {
    let mut c = LruCache::new(2);
    c.put(s("a"), 1);
    c.put(s("b"), 2);
    assert_eq!(c.put(s("a"), 10), None, "updating never evicts");
    assert_eq!(c.len(), 2, "still two keys");
    assert_eq!(c.keys_oldest_first(), vec![s("b"), s("a")], "a is now the newest");
    assert_eq!(c.get("a"), Some(10), "the value was replaced");
    assert_eq!(c.put(s("c"), 3), Some(s("b")), "b is evicted next");
}

#[test]
fn failed_get_changes_nothing() {
    let mut c = LruCache::new(2);
    c.put(s("a"), 1);
    c.put(s("b"), 2);
    c.get("nope");
    assert_eq!(c.keys_oldest_first(), vec![s("a"), s("b")], "a missing key leaves the order alone");
}

#[test]
fn capacity_of_one() {
    let mut c = LruCache::new(1);
    assert_eq!(c.put(s("a"), 1), None, "first fits");
    assert_eq!(c.put(s("b"), 2), Some(s("a")), "second evicts first");
    assert_eq!(c.get("b"), Some(2), "b is stored");
    assert_eq!(c.len(), 1, "one key");
}

#[test]
fn capacity_of_zero_stores_nothing() {
    let mut c = LruCache::new(0);
    assert_eq!(c.put(s("a"), 1), None, "nothing to evict");
    assert_eq!(c.len(), 0, "nothing stored");
    assert_eq!(c.get("a"), None, "nothing to find");
    assert!(c.keys_oldest_first().is_empty(), "no order either");
}

#[test]
fn a_longer_session() {
    let mut c = LruCache::new(3);
    for (i, k) in ["a", "b", "c"].iter().enumerate() {
        c.put(s(k), i as i32);
    }
    c.get("a");
    c.get("b");
    assert_eq!(c.put(s("d"), 9), Some(s("c")), "c was untouched the longest");
    assert_eq!(c.put(s("e"), 8), Some(s("a")), "then a");
    assert_eq!(c.keys_oldest_first(), vec![s("b"), s("d"), s("e")], "final order");
    assert_eq!(c.len(), 3, "full");
}
