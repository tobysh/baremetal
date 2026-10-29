#[test]
fn medians() {
    assert_eq!(median(&[5, 1, 3]), Some(3.0), "odd count, unsorted input");
    assert_eq!(median(&[5, 1, 3, 2]), Some(2.5), "even count");
    assert_eq!(median(&[7]), Some(7.0), "single");
    assert_eq!(median(&[-3, -1]), Some(-2.0), "negatives");
    assert_eq!(median(&[i32::MAX, i32::MAX]), Some(i32::MAX as f64), "no overflow");
    assert_eq!(median(&[]), None, "empty");
}

#[test]
fn median_does_not_modify_input() {
    let v = vec![3, 1, 2];
    median(&v);
    assert_eq!(v, vec![3, 1, 2], "the caller's data must stay in order");
}

#[test]
fn top_n_cases() {
    assert_eq!(top_n(&[4, 9, 1, 9, 3], 3), vec![9, 9, 4], "duplicates count");
    assert_eq!(top_n(&[1, 2], 5), vec![2, 1], "fewer than n");
    assert_eq!(top_n(&[1, 2], 0), Vec::<i32>::new(), "n = 0");
    assert_eq!(top_n(&[-5, -2, -9], 2), vec![-2, -5], "negatives");
    assert_eq!(top_n(&[], 3), Vec::<i32>::new(), "empty");
}

#[test]
fn inserting_in_order() {
    let mut v = vec![1, 3, 5];
    insert_sorted(&mut v, 4);
    assert_eq!(v, vec![1, 3, 4, 5], "middle");
    insert_sorted(&mut v, 0);
    assert_eq!(v, vec![0, 1, 3, 4, 5], "front");
    insert_sorted(&mut v, 9);
    assert_eq!(v, vec![0, 1, 3, 4, 5, 9], "back");
    insert_sorted(&mut v, 3);
    assert_eq!(v, vec![0, 1, 3, 3, 4, 5, 9], "duplicate");
    let mut e: Vec<i32> = vec![];
    insert_sorted(&mut e, 2);
    assert_eq!(e, vec![2], "into empty");
}

#[test]
fn unique_values() {
    assert_eq!(unique_sorted(&[3, 1, 3, 2, 1]), vec![1, 2, 3], "mixed");
    assert_eq!(unique_sorted(&[5, 5, 5]), vec![5], "all equal");
    assert_eq!(unique_sorted(&[-1, 0, -1]), vec![-1, 0], "negatives");
    assert_eq!(unique_sorted(&[]), Vec::<i32>::new(), "empty");
}

#[test]
fn slice_ends() {
    assert_eq!(ends(&[7, 8, 9]), Some((7, 9)), "three");
    assert_eq!(ends(&[1, 2]), Some((1, 2)), "two");
    assert_eq!(ends(&[4]), Some((4, 4)), "one");
    assert_eq!(ends(&[]), None, "none");
}

#[test]
fn people_order() {
    let mut p = vec![
        ("Zed".to_string(), 30),
        ("Amy".to_string(), 30),
        ("Bo".to_string(), 41),
        ("Cy".to_string(), 12),
    ];
    sort_people(&mut p);
    let names: Vec<&str> = p.iter().map(|x| x.0.as_str()).collect();
    assert_eq!(names, vec!["Bo", "Amy", "Zed", "Cy"], "age descending, then name");
    let mut one: Vec<(String, u32)> = vec![];
    sort_people(&mut one);
    assert!(one.is_empty(), "empty stays empty");
}
