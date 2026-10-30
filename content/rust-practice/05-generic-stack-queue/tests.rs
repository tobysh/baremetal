#[test]
fn stack_is_last_in_first_out() {
    let mut s = Stack::new();
    s.push(1);
    s.push(2);
    s.push(3);
    assert_eq!(s.pop(), Some(3), "last pushed comes out first");
    assert_eq!(s.pop(), Some(2), "then 2");
    assert_eq!(s.pop(), Some(1), "then 1");
    assert_eq!(s.pop(), None, "then empty");
}

#[test]
fn stack_peek_does_not_remove() {
    let mut s = Stack::new();
    assert_eq!(s.peek(), None, "peek on empty");
    s.push("a");
    s.push("b");
    assert_eq!(s.peek(), Some(&"b"), "peek sees the top");
    assert_eq!(s.len(), 2, "peek must not remove anything");
}

#[test]
fn stack_len_and_empty() {
    let mut s: Stack<u8> = Stack::new();
    assert!(s.is_empty(), "new stack is empty");
    assert_eq!(s.len(), 0, "len 0");
    s.push(9);
    assert!(!s.is_empty(), "no longer empty");
    assert_eq!(s.len(), 1, "len 1");
    s.pop();
    assert!(s.is_empty(), "empty again");
}

#[test]
fn stack_holds_owned_values() {
    let mut s = Stack::new();
    s.push(String::from("hello"));
    assert_eq!(s.pop().as_deref(), Some("hello"), "owned Strings work");
}

#[test]
fn queue_is_first_in_first_out() {
    let mut q = Queue::new();
    q.enqueue(1);
    q.enqueue(2);
    q.enqueue(3);
    assert_eq!(q.dequeue(), Some(1), "first in, first out");
    assert_eq!(q.dequeue(), Some(2), "then 2");
    assert_eq!(q.dequeue(), Some(3), "then 3");
    assert_eq!(q.dequeue(), None, "then empty");
}

#[test]
fn queue_mixed_operations_keep_order() {
    let mut q = Queue::new();
    q.enqueue('a');
    q.enqueue('b');
    assert_eq!(q.dequeue(), Some('a'), "a first");
    q.enqueue('c');
    q.enqueue('d');
    assert_eq!(q.dequeue(), Some('b'), "b before c");
    assert_eq!(q.dequeue(), Some('c'), "c before d");
    q.enqueue('e');
    assert_eq!(q.dequeue(), Some('d'), "d before e");
    assert_eq!(q.dequeue(), Some('e'), "e last");
    assert_eq!(q.dequeue(), None, "empty");
}

#[test]
fn queue_len_counts_both_vectors() {
    let mut q = Queue::new();
    assert!(q.is_empty(), "new queue is empty");
    q.enqueue(1);
    q.enqueue(2);
    q.enqueue(3);
    q.dequeue();
    q.enqueue(4);
    assert_eq!(q.len(), 3, "three items remain");
    assert!(!q.is_empty(), "not empty");
}

#[test]
fn balanced_accepts_good_input() {
    assert!(balanced(""), "empty is balanced");
    assert!(balanced("()"), "()");
    assert!(balanced("{[()()]}"), "nested mix");
    assert!(balanced("fn main() { let v = [1, 2]; }"), "other characters are ignored");
}

#[test]
fn balanced_rejects_bad_input() {
    assert!(!balanced("(]"), "wrong closer");
    assert!(!balanced("(()"), "unclosed opener");
    assert!(!balanced(")"), "closer with nothing open");
    assert!(!balanced("([)]"), "interleaved");
    assert!(!balanced("}{"), "wrong order");
}
