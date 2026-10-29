fn boom() -> i32 { panic!("boom") }
fn seven() -> i32 { 7 }
fn big() -> u64 { (1..=10u64).product() }

#[test]
fn sums_each_chunk() {
    assert_eq!(sum_chunks(vec![vec![1, 2, 3], vec![10, 20], vec![5]]), vec![6, 30, 5], "each chunk should be summed, in order");
}

#[test]
fn sums_keep_order_with_uneven_work() {
    let big_chunk: Vec<u64> = (1..=200_000).collect();
    let r = sum_chunks(vec![big_chunk, vec![1], vec![2, 2]]);
    assert_eq!(r, vec![20_000_100_000, 1, 4], "results must follow input order, not finish order");
}

#[test]
fn sums_empty_input() {
    assert_eq!(sum_chunks(vec![]), Vec::<u64>::new(), "no chunks means no sums");
    assert_eq!(sum_chunks(vec![vec![]]), vec![0], "an empty chunk sums to 0");
}

#[test]
fn guarded_ok() {
    assert_eq!(run_guarded(seven), Ok(7), "a normal worker should return Ok(7)");
}

#[test]
fn guarded_catches_panic() {
    assert_eq!(run_guarded(boom), Err(String::from("worker panicked")), "a panicking worker should become Err(\"worker panicked\")");
}

#[test]
fn runs_on_another_thread() {
    let (v, id) = on_new_thread(big);
    assert_eq!(v, 3_628_800, "the function's result should be returned");
    assert_ne!(id, std::thread::current().id(), "the work must run on a new thread, not the caller");
}

#[test]
fn each_call_gets_its_own_thread() {
    let (_, a) = on_new_thread(big);
    let (_, b) = on_new_thread(big);
    assert_ne!(a, b, "two spawned threads should have different ids");
}
