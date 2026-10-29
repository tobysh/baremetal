#[test]
fn lockstep_values_match_the_round() {
    let r = lockstep(4, 3);
    assert_eq!(r.len(), 4, "one result vector per thread");
    for (t, seen) in r.iter().enumerate() {
        assert_eq!(seen, &vec![4, 8, 12], "thread {t} should see n*(r+1) in each round");
    }
}

#[test]
fn lockstep_single_thread_and_no_rounds() {
    assert_eq!(lockstep(1, 3), vec![vec![1, 2, 3]], "a barrier of one never blocks");
    assert_eq!(lockstep(3, 0), vec![Vec::<usize>::new(); 3], "zero rounds, nothing read");
}

#[test]
fn lockstep_many_threads() {
    let r = lockstep(8, 5);
    assert!(r.iter().all(|v| v == &vec![8, 16, 24, 32, 40]), "8 threads, 5 rounds");
}

#[test]
fn exactly_one_leader_per_round() {
    assert_eq!(leader_count(5, 4), 4, "one leader in each of the 4 rounds");
    assert_eq!(leader_count(1, 3), 3, "with one thread it leads every round");
    assert_eq!(leader_count(6, 0), 0);
}

#[test]
fn init_runs_exactly_once() {
    let handles: Vec<_> = (0..8)
        .map(|_| std::thread::spawn(|| { for _ in 0..50 { ensure_init(); } }))
        .collect();
    for h in handles { h.join().unwrap(); }
    ensure_init();
    assert_eq!(INIT_RUNS.load(Ordering::SeqCst), 1, "the initialiser must run once in total");
}

#[test]
fn config_is_built_once_and_shared() {
    let handles: Vec<_> = (0..8)
        .map(|_| std::thread::spawn(|| config() as *const String as usize))
        .collect();
    let addrs: Vec<usize> = handles.into_iter().map(|h| h.join().unwrap()).collect();
    assert!(addrs.windows(2).all(|w| w[0] == w[1]), "every thread must see the same stored value");
    assert_eq!(config(), "config-v1");
    assert_eq!(CONFIG_BUILDS.load(Ordering::SeqCst), 1, "the builder closure must run exactly once");
}
