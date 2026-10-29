#[test]
fn set_get_incr() {
    let r = Registry::new();
    assert_eq!(r.get("x"), None, "missing key");
    r.set("x", 5);
    assert_eq!(r.get("x"), Some(5));
    assert_eq!(r.incr("x"), 6, "incr returns the new value");
    assert_eq!(r.incr("fresh"), 1, "a new key starts from 0");
    r.set("x", 1);
    assert_eq!(r.get("x"), Some(1), "set overwrites");
}

#[test]
fn snapshot_is_sorted() {
    let r = Registry::new();
    r.set("pear", 3);
    r.set("apple", 1);
    r.set("mango", 2);
    assert_eq!(
        r.snapshot(),
        vec![("apple".to_string(), 1), ("mango".to_string(), 2), ("pear".to_string(), 3)],
        "entries sorted by key"
    );
    assert!(Registry::new().snapshot().is_empty(), "empty registry, empty snapshot");
}

#[test]
fn handles_share_state() {
    let a = Registry::new();
    let b = a.handle();
    b.set("k", 9);
    assert_eq!(a.get("k"), Some(9), "a write through one handle is visible through the other");
}

#[test]
fn concurrent_incr_is_atomic() {
    let r = Registry::new();
    let handles: Vec<_> = (0..8)
        .map(|_| {
            let h = r.handle();
            std::thread::spawn(move || { for _ in 0..250 { h.incr("hits"); } })
        })
        .collect();
    for h in handles { h.join().unwrap(); }
    assert_eq!(r.get("hits"), Some(2000), "8 x 250 increments of one key");
}

#[test]
fn readers_and_a_writer_together() {
    let r = Registry::new();
    r.set("n", 0);
    let writer = {
        let h = r.handle();
        std::thread::spawn(move || { for _ in 0..200 { h.incr("n"); } })
    };
    let readers: Vec<_> = (0..4)
        .map(|_| {
            let h = r.handle();
            std::thread::spawn(move || {
                let mut last = 0;
                for _ in 0..200 {
                    let v = h.get("n").unwrap();
                    assert!(v >= last, "a counter must never appear to go backwards");
                    last = v;
                }
            })
        })
        .collect();
    writer.join().unwrap();
    for t in readers { t.join().unwrap(); }
    assert_eq!(r.get("n"), Some(200));
}

#[test]
fn can_read_now_reflects_lock_state() {
    let lock = RwLock::new(1);
    assert!(can_read_now(&lock), "an idle lock can be read");
    {
        let _r1 = lock.read().unwrap();
        assert!(can_read_now(&lock), "readers share the lock");
    }
    {
        let _w = lock.write().unwrap();
        assert!(!can_read_now(&lock), "a held write lock excludes readers");
    }
    assert!(can_read_now(&lock), "released again");
}
