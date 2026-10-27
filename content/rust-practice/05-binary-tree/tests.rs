fn build(values: &[i32]) -> Tree {
    let mut t = Tree::new();
    for &v in values {
        t.insert(v);
    }
    t
}

#[test]
fn empty_tree() {
    let t = Tree::new();
    assert!(!t.contains(1), "nothing in an empty tree");
    assert_eq!(t.in_order(), Vec::<i32>::new(), "empty traversal");
    assert_eq!(t.height(), 0, "empty height");
    assert_eq!(t.min(), None, "empty min");
}

#[test]
fn insert_reports_new_values() {
    let mut t = Tree::new();
    assert!(t.insert(5), "first insert is new");
    assert!(t.insert(3), "3 is new");
    assert!(!t.insert(5), "5 is a duplicate");
    assert!(!t.insert(3), "3 is a duplicate");
    assert_eq!(t.in_order(), vec![3, 5], "no duplicates stored");
}

#[test]
fn contains_finds_only_inserted() {
    let t = build(&[50, 30, 70, 20, 40, 60, 80]);
    for v in [50, 30, 70, 20, 40, 60, 80] {
        assert!(t.contains(v), "should contain {}", v);
    }
    for v in [0, 25, 55, 100] {
        assert!(!t.contains(v), "should not contain {}", v);
    }
}

#[test]
fn in_order_is_sorted() {
    let t = build(&[5, 2, 8, 1, 9, 3, 7, -4, 0]);
    assert_eq!(t.in_order(), vec![-4, 0, 1, 2, 3, 5, 7, 8, 9], "values should come out sorted");
}

#[test]
fn height_of_balanced_and_lopsided() {
    assert_eq!(build(&[1]).height(), 1, "one node");
    assert_eq!(build(&[2, 1, 3]).height(), 2, "balanced three");
    assert_eq!(build(&[1, 2, 3, 4, 5]).height(), 5, "a chain to the right");
    assert_eq!(build(&[5, 4, 3, 2, 1]).height(), 5, "a chain to the left");
}

#[test]
fn min_follows_the_left_edge() {
    assert_eq!(build(&[5]).min(), Some(5), "single node");
    assert_eq!(build(&[5, 2, 8, -3, 1]).min(), Some(-3), "smallest value");
    assert_eq!(build(&[1, 2, 3]).min(), Some(1), "root is smallest");
}

#[test]
fn many_values() {
    let mut t = Tree::new();
    for i in 0..1000 {
        // 37 is coprime with 1009, which spreads values out
        t.insert((i * 37) % 1009);
    }
    let v = t.in_order();
    assert_eq!(v.len(), 1000, "all values are distinct");
    assert!(v.windows(2).all(|w| w[0] < w[1]), "traversal must be strictly ascending");
    assert!(t.contains(37), "37 was inserted");
}
