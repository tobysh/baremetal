use std::sync::atomic::{AtomicUsize, Ordering};

fn strs(v: &[&str]) -> Vec<String> {
    v.iter().map(|s| s.to_string()).collect()
}

#[test]
fn guarded_normal_return() {
    assert_eq!(run_guarded(|| 6 * 7), Ok(42), "a value comes back");
    let data = vec![1, 2, 3, 4];
    assert_eq!(run_guarded(move || data.iter().sum()), Ok(10), "the closure may own data");
}

#[test]
fn guarded_panics_become_errors() {
    assert_eq!(run_guarded(|| panic!("boom")), Err("boom".to_string()), "a literal panic message");
    let n = 7;
    assert_eq!(run_guarded(move || panic!("bad value {}", n)), Err("bad value 7".to_string()), "a formatted panic message");
    assert_eq!(run_guarded(|| std::panic::panic_any(5u8)), Err("unknown panic".to_string()), "a payload that is neither str nor String");
}

#[test]
fn guarded_arithmetic_panic() {
    let zero = std::hint::black_box(0);
    let r = run_guarded(move || 10 / zero);
    assert!(r.is_err(), "division by zero panics and is reported");
}

#[test]
fn ordered_success() {
    let r = collect_ordered(6, Arc::new(|i| Ok(i as u32 * i as u32)));
    assert_eq!(r, Ok(vec![0, 1, 4, 9, 16, 25]), "results in index order");
    assert_eq!(collect_ordered(0, Arc::new(|_| Ok(1))), Ok(vec![]), "no workers");
}

#[test]
fn ordered_reports_the_lowest_failing_index() {
    for _ in 0..20 {
        let r = collect_ordered(8, Arc::new(|i| if i == 2 || i == 5 { Err(format!("bad {i}")) } else { Ok(i as u32) }));
        assert_eq!(r, Err("bad 2".to_string()), "index 2 fails before index 5, however the threads are scheduled");
    }
}

#[test]
fn ordered_panics_and_all_workers_run() {
    let ran = Arc::new(AtomicUsize::new(0));
    let counter = Arc::clone(&ran);
    let r = collect_ordered(
        10,
        Arc::new(move |i| {
            counter.fetch_add(1, Ordering::SeqCst);
            if i == 0 {
                Err("first failed".to_string())
            } else {
                Ok(i as u32)
            }
        }),
    );
    assert_eq!(r, Err("first failed".to_string()), "worker 0 failed");
    assert_eq!(ran.load(Ordering::SeqCst), 10, "every worker finished before the function returned");

    let r = collect_ordered(4, Arc::new(|i| if i == 3 { panic!("kaboom") } else { Ok(i as u32) }));
    assert_eq!(r, Err("worker panicked".to_string()), "a panicking worker");

    let r = collect_ordered(4, Arc::new(|i| if i == 1 { panic!("kaboom") } else if i == 3 { Err("late error".to_string()) } else { Ok(0) }));
    assert_eq!(r, Err("worker panicked".to_string()), "the lowest index (a panic) wins over a later error");
}

#[test]
fn parse_chunks_success() {
    let r = parse_chunks(vec![strs(&["1", " 2 "]), strs(&["-3"]), vec![], strs(&["40", "50"])]);
    assert_eq!(r, Ok(vec![1, 2, -3, 40, 50]), "chunk by chunk, in order");
    assert_eq!(parse_chunks(vec![]), Ok(vec![]), "no chunks");
}

#[test]
fn parse_chunks_reports_the_lowest_bad_chunk() {
    for _ in 0..20 {
        let r = parse_chunks(vec![strs(&["1"]), strs(&["2", "x", "y"]), strs(&["z"])]);
        assert_eq!(r, Err("chunk 1: invalid digit found in string".to_string()), "chunk 1 is the lowest bad chunk");
    }
    let r = parse_chunks(vec![strs(&[""]), strs(&["q"])]);
    assert_eq!(r, Err("chunk 0: cannot parse integer from empty string".to_string()), "an empty string in chunk 0");
}
