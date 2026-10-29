#[test]
fn tally_counts_threads() {
    assert_eq!(tally_on_threads(4), (4, 4), "4 threads: total 4, 4 distinct ids");
    assert_eq!(tally_on_threads(10), (10, 10), "10 threads: total 10, 10 distinct ids");
}

#[test]
fn tally_zero_and_one() {
    assert_eq!(tally_on_threads(0), (0, 0), "no threads, nothing recorded");
    assert_eq!(tally_on_threads(1), (1, 1), "one worker thread");
}

#[test]
fn round_trip_returns_value_from_other_thread() {
    let (v, id) = round_trip(String::from("payload"));
    assert_eq!(v, "payload", "the value should come back unchanged");
    assert_ne!(id, thread::current().id(), "it must have been held by a different thread");
}

#[test]
fn round_trip_works_for_collections() {
    let (v, id) = round_trip(vec![1, 2, 3]);
    assert_eq!(v, vec![1, 2, 3]);
    assert_ne!(id, thread::current().id(), "different thread again");
}

#[test]
fn round_trip_with_arc_mutex() {
    let shared = Arc::new(Mutex::new(5));
    let (s, id) = round_trip(Arc::clone(&shared));
    *s.lock().unwrap() += 1;
    assert_eq!(*shared.lock().unwrap(), 6, "an Arc<Mutex<_>> is Send and still points at the same data");
    assert_ne!(id, thread::current().id());
}
