fn build_back(values: &[i32]) -> List {
    let mut l = List::new();
    for &v in values {
        l.push_back(v);
    }
    l
}

#[test]
fn empty_list() {
    let mut l = List::new();
    assert_eq!(l.len(), 0, "empty length");
    assert_eq!(l.peek(), None, "empty peek");
    assert_eq!(l.pop_front(), None, "empty pop");
    assert_eq!(l.to_vec(), Vec::<i32>::new(), "empty to_vec");
    assert!(!l.remove(1), "nothing to remove");
}

#[test]
fn push_front_stacks_up() {
    let mut l = List::new();
    for v in [1, 2, 3] {
        l.push_front(v);
    }
    assert_eq!(l.to_vec(), vec![3, 2, 1], "last pushed is first");
    assert_eq!(l.len(), 3, "three nodes");
    assert_eq!(l.peek(), Some(&3), "peek sees the head");
}

#[test]
fn push_back_keeps_order() {
    let l = build_back(&[10, -2, 7, 7]);
    assert_eq!(l.to_vec(), vec![10, -2, 7, 7], "order preserved, duplicates kept");
    assert_eq!(l.len(), 4, "four nodes");
}

#[test]
fn mixed_pushes() {
    let mut l = List::new();
    l.push_back(2);
    l.push_front(1);
    l.push_back(3);
    l.push_front(0);
    assert_eq!(l.to_vec(), vec![0, 1, 2, 3], "front and back pushes combine");
}

#[test]
fn pop_front_drains_in_order() {
    let mut l = build_back(&[4, 5, 6]);
    assert_eq!(l.pop_front(), Some(4), "first pop");
    assert_eq!(l.peek(), Some(&5), "head moved on");
    assert_eq!(l.pop_front(), Some(5), "second pop");
    assert_eq!(l.pop_front(), Some(6), "third pop");
    assert_eq!(l.pop_front(), None, "empty again");
    assert_eq!(l.len(), 0, "length back to zero");
    l.push_back(9);
    assert_eq!(l.to_vec(), vec![9], "list is reusable after emptying");
}

#[test]
fn reverse_flips_order() {
    let mut l = build_back(&[1, 2, 3, 4]);
    l.reverse();
    assert_eq!(l.to_vec(), vec![4, 3, 2, 1], "four items");
    l.reverse();
    assert_eq!(l.to_vec(), vec![1, 2, 3, 4], "reversing twice restores it");
    let mut one = build_back(&[7]);
    one.reverse();
    assert_eq!(one.to_vec(), vec![7], "single item");
    let mut none = List::new();
    none.reverse();
    assert_eq!(none.len(), 0, "empty stays empty");
    let mut l2 = build_back(&[1, 2, 3]);
    l2.reverse();
    l2.push_back(0);
    assert_eq!(l2.to_vec(), vec![3, 2, 1, 0], "list is still well formed after reverse");
}

#[test]
fn remove_head_middle_tail() {
    let mut l = build_back(&[1, 2, 3, 4, 5]);
    assert!(l.remove(3), "middle");
    assert_eq!(l.to_vec(), vec![1, 2, 4, 5], "3 gone");
    assert!(l.remove(1), "head");
    assert_eq!(l.to_vec(), vec![2, 4, 5], "1 gone");
    assert!(l.remove(5), "tail");
    assert_eq!(l.to_vec(), vec![2, 4], "5 gone");
    assert!(!l.remove(99), "absent value");
    assert_eq!(l.len(), 2, "unchanged by failed remove");
}

#[test]
fn remove_only_first_match() {
    let mut l = build_back(&[2, 7, 2, 7]);
    assert!(l.remove(7), "first 7");
    assert_eq!(l.to_vec(), vec![2, 2, 7], "only the first 7 goes");
    assert!(l.remove(2), "first 2");
    assert_eq!(l.to_vec(), vec![2, 7], "only the first 2 goes");
    let mut s = build_back(&[5]);
    assert!(s.remove(5), "only node");
    assert_eq!(s.len(), 0, "list is empty");
    assert_eq!(s.peek(), None, "head is None");
}

#[test]
fn longer_list() {
    let mut l = List::new();
    for i in 0..1000 {
        l.push_back(i);
    }
    assert_eq!(l.len(), 1000, "1000 nodes");
    l.reverse();
    assert_eq!(l.peek(), Some(&999), "reversed head");
    assert!(l.remove(500), "remove from the middle");
    assert_eq!(l.len(), 999, "one fewer");
    let v = l.to_vec();
    assert!(v.windows(2).all(|w| w[0] > w[1]), "still strictly descending");
}
