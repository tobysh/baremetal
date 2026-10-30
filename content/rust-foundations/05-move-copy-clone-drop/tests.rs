fn new_log() -> Log {
    Rc::new(RefCell::new(Vec::new()))
}

#[test]
fn guard_logs_when_it_goes_out_of_scope() {
    let log = new_log();
    {
        let _g = Guard::new("alpha", &log);
        assert!(log.borrow().is_empty(), "nothing is dropped while the guard is alive");
    }
    assert_eq!(*log.borrow(), vec!["drop alpha".to_string()], "leaving the block should drop the guard");
}

#[test]
fn locals_drop_in_reverse_order() {
    let log = new_log();
    {
        let _x = Guard::new("x", &log);
        let _y = Guard::new("y", &log);
        let _z = Guard::new("z", &log);
    }
    let expect: Vec<String> = ["drop z", "drop y", "drop x"].iter().map(|s| s.to_string()).collect();
    assert_eq!(*log.borrow(), expect, "the last created guard is dropped first");
}

#[test]
fn moving_a_guard_into_a_function_drops_it_there() {
    fn eat(_g: Guard) {}
    let log = new_log();
    let g = Guard::new("moved", &log);
    eat(g);
    assert_eq!(*log.borrow(), vec!["drop moved".to_string()], "the guard is dropped when eat() returns");
}

#[test]
fn run_scope_orders_events() {
    let log = new_log();
    run_scope(&log, "a", "b");
    let expect: Vec<String> = ["drop a", "work", "drop b"].iter().map(|s| s.to_string()).collect();
    assert_eq!(*log.borrow(), expect, "run_scope(\"a\", \"b\")");
}

#[test]
fn run_scope_with_other_names() {
    let log = new_log();
    run_scope(&log, "file", "lock");
    run_scope(&log, "x1", "y2");
    let expect: Vec<String> = ["drop file", "work", "drop lock", "drop x1", "work", "drop y2"]
        .iter().map(|s| s.to_string()).collect();
    assert_eq!(*log.borrow(), expect, "two calls into the same log");
}

#[test]
fn clone_all_makes_independent_copies() {
    let original: Vec<String> = vec!["a".to_string(), "bb".to_string(), "".to_string()];
    let mut copy = clone_all(&original);
    assert_eq!(copy, original, "the copy should have the same items in order");
    copy[0].push('!');
    assert_eq!(original[0], "a", "changing the copy must not change the original");
    assert!(clone_all(&[]).is_empty(), "empty in, empty out");
}

#[test]
fn with_added_appends() {
    let v = with_added(vec!["one".to_string()], "two".to_string());
    assert_eq!(v, vec!["one".to_string(), "two".to_string()], "extra goes to the end");
    let v2 = with_added(Vec::new(), "solo".to_string());
    assert_eq!(v2, vec!["solo".to_string()], "adding to an empty vector");
    let v3 = with_added(v, "three".to_string());
    assert_eq!(v3.len(), 3, "three items after a second call");
    assert_eq!(v3[2], "three", "newest item last");
}
