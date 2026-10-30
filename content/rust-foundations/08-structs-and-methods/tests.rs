#[test]
fn new_stack_is_empty() {
    let s = Stack::new();
    assert!(s.is_empty(), "a new stack should be empty");
    assert_eq!(s.len(), 0, "a new stack should have len 0");
}

#[test]
fn push_increases_len() {
    let mut s = Stack::new();
    s.push(1);
    s.push(2);
    assert_eq!(s.len(), 2, "after two pushes len should be 2");
    assert!(!s.is_empty(), "stack with items should not be empty");
}

#[test]
fn pop_returns_items_in_reverse_order() {
    let mut s = Stack::new();
    s.push(1);
    s.push(2);
    s.push(3);
    assert_eq!(s.pop(), Some(3), "first pop should return Some(3)");
    assert_eq!(s.pop(), Some(2), "second pop should return Some(2)");
    assert_eq!(s.pop(), Some(1), "third pop should return Some(1)");
}

#[test]
fn pop_on_empty_stack_is_none() {
    let mut s = Stack::new();
    assert_eq!(s.pop(), None, "pop on an empty stack should be None");
    s.push(9);
    s.pop();
    assert_eq!(s.pop(), None, "pop after emptying the stack should be None");
}

#[test]
fn peek_shows_top_without_removing() {
    let mut s = Stack::new();
    s.push(5);
    s.push(6);
    assert_eq!(s.peek(), Some(6), "peek should return Some(6)");
    assert_eq!(s.peek(), Some(6), "peek must not remove the item");
    assert_eq!(s.len(), 2, "peek must not change len");
}

#[test]
fn peek_on_empty_stack_is_none() {
    let s = Stack::new();
    assert_eq!(s.peek(), None, "peek on an empty stack should be None");
}

#[test]
fn len_goes_down_after_pop() {
    let mut s = Stack::new();
    s.push(1);
    s.push(2);
    s.pop();
    assert_eq!(s.len(), 1, "len should be 1 after push, push, pop");
    s.pop();
    assert!(s.is_empty(), "stack should be empty after popping everything");
}

#[test]
fn handles_negative_numbers() {
    let mut s = Stack::new();
    s.push(-7);
    assert_eq!(s.peek(), Some(-7), "peek should return Some(-7)");
    assert_eq!(s.pop(), Some(-7), "pop should return Some(-7)");
}
