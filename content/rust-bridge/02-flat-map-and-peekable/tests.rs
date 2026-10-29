#[test]
fn runs() {
    assert_eq!(run_lengths("aaabcc"), vec![('a', 3), ('b', 1), ('c', 2)], "aaabcc");
    assert_eq!(run_lengths("abc"), vec![('a', 1), ('b', 1), ('c', 1)], "no repeats");
    assert_eq!(run_lengths("zzzz"), vec![('z', 4)], "one long run");
    assert_eq!(run_lengths("abab"), vec![('a', 1), ('b', 1), ('a', 1), ('b', 1)], "alternating");
    assert_eq!(run_lengths(""), Vec::<(char, usize)>::new(), "empty");
}

#[test]
fn runs_handle_unicode() {
    assert_eq!(run_lengths("ééx"), vec![('é', 2), ('x', 1)], "multi-byte characters count once");
}

#[test]
fn words_are_flattened_and_lowercased() {
    let v = all_words(&["Hello World", "  again  ", "", "ONE two Three"]);
    assert_eq!(v, vec!["hello", "world", "again", "one", "two", "three"], "multi-line input");
    assert_eq!(all_words(&[]), Vec::<String>::new(), "no lines");
    assert_eq!(all_words(&["   "]), Vec::<String>::new(), "only spaces");
}

#[test]
fn partition_by_sign() {
    assert_eq!(split_by_sign(&[3, -1, 0, -7, 2]), (vec![3, 0, 2], vec![-1, -7]), "mixed");
    assert_eq!(split_by_sign(&[-1, -2]), (vec![], vec![-1, -2]), "all negative");
    assert_eq!(split_by_sign(&[5, 6]), (vec![5, 6], vec![]), "all non-negative");
    assert_eq!(split_by_sign(&[]), (vec![], vec![]), "empty");
}

#[test]
fn find_the_first_big_square() {
    assert_eq!(first_big_square(&[1, 2, 5, 9], 20), Some(25), "5 squared is first above 20");
    assert_eq!(first_big_square(&[-6, 2], 30), Some(36), "negative numbers square positive");
    assert_eq!(first_big_square(&[1, 2, 3], 9), None, "9 is not greater than 9");
    assert_eq!(first_big_square(&[], 0), None, "empty");
    assert_eq!(first_big_square(&[0, 1], -1), Some(0), "0 > -1");
}

#[test]
fn parse_what_parses() {
    assert_eq!(numbers_in(&["12", "x1", "-4", "", " 7", "+3"]), vec![12, -4, 3], "bad items are skipped");
    assert_eq!(numbers_in(&["a", "b"]), Vec::<i32>::new(), "nothing parses");
    assert_eq!(numbers_in(&["99999999999", "1"]), vec![1], "overflow does not parse");
}
