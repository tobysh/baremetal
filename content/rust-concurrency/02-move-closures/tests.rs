#[test]
fn greets_in_order() {
    let r = greet_all(vec!["Ada".to_string(), "Linus".to_string(), "Grace".to_string()]);
    assert_eq!(r, vec!["hello, Ada", "hello, Linus", "hello, Grace"], "greetings should follow the input order");
}

#[test]
fn greets_nobody() {
    assert!(greet_all(vec![]).is_empty(), "no names, no greetings");
}

#[test]
fn greets_many() {
    let names: Vec<String> = (0..20).map(|i| format!("n{i}")).collect();
    let r = greet_all(names);
    assert_eq!(r.len(), 20, "one greeting per name");
    assert_eq!(r[13], "hello, n13", "the 14th greeting should be for n13");
}

#[test]
fn squares() {
    assert_eq!(indexed_squares(6), vec![0, 1, 4, 9, 16, 25], "thread i should return i*i");
    assert_eq!(indexed_squares(0), Vec::<usize>::new(), "zero threads, empty result");
}

#[test]
fn range_sum_basic() {
    let h = spawn_range_sum(1, 4);
    assert_eq!(h.join().unwrap(), 10, "1+2+3+4 should be 10");
}

#[test]
fn range_sum_other_inputs() {
    assert_eq!(spawn_range_sum(100, 3).join().unwrap(), 303, "100+101+102 should be 303");
    assert_eq!(spawn_range_sum(7, 0).join().unwrap(), 0, "an empty range sums to 0");
}

#[test]
fn handles_can_overlap() {
    let a = spawn_range_sum(0, 1000);
    let b = spawn_range_sum(1000, 1000);
    assert_eq!(a.join().unwrap() + b.join().unwrap(), 1_999_000 - 0, "two handles held at once should both work");
}
