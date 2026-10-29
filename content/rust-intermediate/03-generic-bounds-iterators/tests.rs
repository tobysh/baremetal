#[test]
fn countdown_yields_in_order() {
    assert_eq!(Countdown { n: 4 }.collect::<Vec<_>>(), vec![4, 3, 2, 1], "countdown from 4");
    assert_eq!(Countdown { n: 1 }.collect::<Vec<_>>(), vec![1], "countdown from 1");
    assert_eq!(Countdown { n: 0 }.count(), 0, "countdown from 0 is empty");
}

#[test]
fn countdown_works_with_adapters() {
    assert_eq!(Countdown { n: 100 }.sum::<u32>(), 5050, "sum of 1..=100");
    let evens: Vec<u32> = Countdown { n: 9 }.filter(|x| x % 2 == 0).collect();
    assert_eq!(evens, vec![8, 6, 4, 2], "adapters come for free");
}

#[test]
fn countdown_stays_finished() {
    let mut c = Countdown { n: 1 };
    assert_eq!(c.next(), Some(1), "first");
    assert_eq!(c.next(), None, "then done");
    assert_eq!(c.next(), None, "and still done");
}

#[test]
fn largest_by_key_strings() {
    let words = ["pear", "fig", "banana", "apple"];
    assert_eq!(largest_by_key(&words, |s| s.len()), Some(&"banana"), "longest word");
    assert_eq!(largest_by_key(&words, |s| s.chars().last().unwrap()), Some(&"pear"), "greatest last letter is r");
}

#[test]
fn largest_by_key_tie_and_empty() {
    let pairs = [("a", 3), ("b", 3), ("c", 1)];
    assert_eq!(largest_by_key(&pairs, |p| p.1), Some(&("a", 3)), "the first of a tie");
    let none: [i32; 0] = [];
    assert_eq!(largest_by_key(&none, |x| *x), None, "nothing to choose from");
}

#[test]
fn largest_by_key_floats_and_negated_keys() {
    let v = [3.5, 1.25, 2.0];
    assert_eq!(largest_by_key(&v, |x| -*x), Some(&1.25), "the key can reverse the order");
    assert_eq!(largest_by_key(&v, |x| *x), Some(&3.5), "plain float keys");
}

#[test]
fn summarize_many_types() {
    assert_eq!(summarize(vec![1, 2, 3]), "1, 2, 3", "integers");
    assert_eq!(summarize(["a", "b"]), "a, b", "strs");
    assert_eq!(summarize(vec![2.5, 4.0]), "2.5, 4", "floats");
    assert_eq!(summarize(Vec::<i32>::new()), "", "nothing");
    assert_eq!(summarize(Countdown { n: 3 }), "3, 2, 1", "your own iterator");
    assert_eq!(summarize(7..10), "7, 8, 9", "a range");
}

#[test]
fn every_other_items() {
    assert_eq!(every_other(Countdown { n: 5 }), vec![5, 3, 1], "odd count");
    assert_eq!(every_other("abcde".chars()), vec!['a', 'c', 'e'], "chars");
    assert_eq!(every_other(vec![10, 20, 30, 40].into_iter()), vec![10, 30], "even count");
    assert_eq!(every_other(std::iter::empty::<u8>()), Vec::<u8>::new(), "nothing");
}
