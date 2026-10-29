#[test]
fn atomic_counter_is_exact() {
    assert_eq!(count_atomic(4, 1000), 4000);
    assert_eq!(count_atomic(8, 10_000), 80_000, "no lost updates");
    assert_eq!(count_atomic(0, 5), 0);
}

#[test]
fn ids_are_unique_across_threads() {
    let handles: Vec<_> = (0..6)
        .map(|_| std::thread::spawn(|| (0..200).map(|_| next_id()).collect::<Vec<u64>>()))
        .collect();
    let mut all: Vec<u64> = handles.into_iter().flat_map(|h| h.join().unwrap()).collect();
    assert_eq!(all.len(), 1200);
    all.sort();
    all.dedup();
    assert_eq!(all.len(), 1200, "every id must be unique");
}

#[test]
fn ids_increase_on_one_thread() {
    let a = next_id();
    let b = next_id();
    assert!(b > a, "later ids are larger on the same thread ({a} then {b})");
}

#[test]
fn record_max_keeps_the_largest() {
    let m = AtomicI64::new(i64::MIN);
    record_max(&m, 5);
    record_max(&m, -3);
    record_max(&m, 9);
    record_max(&m, 7);
    assert_eq!(m.load(Ordering::SeqCst), 9);
}

#[test]
fn record_max_concurrently() {
    let m = Arc::new(AtomicI64::new(0));
    let handles: Vec<_> = (0..8i64)
        .map(|t| {
            let m = Arc::clone(&m);
            std::thread::spawn(move || {
                for i in 0..500 { record_max(&m, t * 1000 + i); }
            })
        })
        .collect();
    for h in handles { h.join().unwrap(); }
    assert_eq!(m.load(Ordering::SeqCst), 7499, "the largest value written by any thread wins");
}

#[test]
fn stop_flag_toggles() {
    let s = Stop::new();
    assert!(!s.is_requested(), "starts clear");
    s.request();
    assert!(s.is_requested(), "set after request");
}

#[test]
fn worker_runs_until_stopped() {
    let stop = Arc::new(Stop::new());
    let h = spawn_counter(Arc::clone(&stop));
    std::thread::sleep(std::time::Duration::from_millis(30));
    assert!(!h.is_finished(), "the worker must keep running until asked to stop");
    stop.request();
    let rounds = h.join().unwrap();
    assert!(rounds >= 1, "the worker must have completed at least one round");
}

#[test]
fn worker_stops_immediately_if_already_requested() {
    let stop = Arc::new(Stop::new());
    stop.request();
    let rounds = spawn_counter(Arc::clone(&stop)).join().unwrap();
    assert!(rounds >= 1, "even a pre-stopped worker runs one round");
}
