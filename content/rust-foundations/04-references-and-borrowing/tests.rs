#[test]
fn swap_exchanges_values() {
    let (mut a, mut b) = (3, -7);
    swap(&mut a, &mut b);
    assert_eq!((a, b), (-7, 3), "swap(3, -7) should give (-7, 3)");
    let (mut c, mut d) = (0, 0);
    swap(&mut c, &mut d);
    assert_eq!((c, d), (0, 0), "swapping equal values changes nothing");
}

#[test]
fn swap_works_on_vector_elements_via_separate_borrows() {
    let mut v = vec![10, 20];
    let (x, y) = v.split_at_mut(1);
    swap(&mut x[0], &mut y[0]);
    assert_eq!(v, vec![20, 10], "swap should write through the references");
}

#[test]
fn double_all_changes_in_place() {
    let mut v = vec![1, -2, 0, 50];
    double_all(&mut v);
    assert_eq!(v, vec![2, -4, 0, 100], "every element should be doubled");
    let mut e: Vec<i32> = Vec::new();
    double_all(&mut e);
    assert!(e.is_empty(), "an empty vector stays empty");
}

#[test]
fn count_matching_counts_exact_matches() {
    let words: Vec<String> = ["cat", "dog", "cat", "Cat", "bird"].iter().map(|s| s.to_string()).collect();
    assert_eq!(count_matching(&words, "cat"), 2, "two lowercase cats");
    assert_eq!(count_matching(&words, "Cat"), 1, "case matters");
    assert_eq!(count_matching(&words, "fish"), 0, "no fish");
    let empty: Vec<String> = Vec::new();
    assert_eq!(count_matching(&empty, "x"), 0, "empty list");
    assert_eq!(words.len(), 5, "the caller still owns the words");
}

#[test]
fn largest_mut_writes_through() {
    let mut v = vec![4, 17, 9];
    if let Some(m) = largest_mut(&mut v) {
        *m += 100;
    }
    assert_eq!(v, vec![4, 117, 9], "the largest element (17) should be changed in the vector");
    let mut w = vec![-5, -2, -9];
    *largest_mut(&mut w).expect("not empty") = 0;
    assert_eq!(w, vec![-5, 0, -9], "-2 is the largest here");
}

#[test]
fn largest_mut_on_empty_is_none() {
    let mut v: Vec<i32> = Vec::new();
    assert!(largest_mut(&mut v).is_none(), "an empty vector has no largest element");
    let mut one = vec![8];
    assert_eq!(largest_mut(&mut one).map(|x| *x), Some(8), "single element");
}
