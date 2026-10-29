#[test]
fn sums_in_parts() {
    let v: Vec<u64> = (1..=100).collect();
    assert_eq!(parallel_sum(&v, 4), 5050, "1..=100 summed in 4 parts");
    assert_eq!(parallel_sum(&v, 1), 5050, "one part");
    assert_eq!(parallel_sum(&v, 7), 5050, "7 parts do not divide 100 evenly");
    assert_eq!(parallel_sum(&v, 500), 5050, "more parts than items");
}

#[test]
fn sum_edge_cases() {
    assert_eq!(parallel_sum(&[], 4), 0, "empty slice");
    assert_eq!(parallel_sum(&[5], 0), 5, "parts == 0 acts like 1");
}

#[test]
fn sum_borrows_without_consuming() {
    let v: Vec<u64> = vec![10, 20, 30];
    let a = parallel_sum(&v, 2);
    let b = parallel_sum(&v, 3);
    assert_eq!((a, b), (60, 60), "the data is only borrowed, so it can be used again");
    assert_eq!(v.len(), 3);
}

#[test]
fn increments_every_element() {
    let mut v: Vec<i32> = (0..10).collect();
    increment_all(&mut v, 3);
    assert_eq!(v, (1..=10).collect::<Vec<i32>>(), "every element +1, none skipped or doubled");
}

#[test]
fn increment_edge_cases() {
    let mut empty: Vec<i32> = Vec::new();
    increment_all(&mut empty, 4);
    assert!(empty.is_empty());
    let mut one = vec![41];
    increment_all(&mut one, 0);
    assert_eq!(one, vec![42], "parts == 0 acts like 1");
    let mut many = vec![0; 1000];
    increment_all(&mut many, 8);
    assert!(many.iter().all(|&x| x == 1), "each element incremented exactly once");
}

#[test]
fn min_and_max() {
    assert_eq!(min_max(&[3, -7, 12, 0, 5]), Some((-7, 12)));
    assert_eq!(min_max(&[4]), Some((4, 4)), "single element is both");
    assert_eq!(min_max(&[]), None, "empty slice has no min or max");
}
