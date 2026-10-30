struct Node {
    value: i32,
    next: Option<Box<Node>>,
}

struct List {
    head: Option<Box<Node>>,
}

impl List {
    fn new() -> List {
        List { head: None }
    }

    fn push_front(&mut self, value: i32) {
        // your code here
    }

    fn push_back(&mut self, value: i32) {
        // your code here
    }

    fn pop_front(&mut self) -> Option<i32> {
        None
    }

    fn peek(&self) -> Option<&i32> {
        None
    }

    fn len(&self) -> usize {
        0
    }

    fn to_vec(&self) -> Vec<i32> {
        Vec::new()
    }

    fn reverse(&mut self) {
        // your code here
    }

    fn remove(&mut self, value: i32) -> bool {
        false
    }
}

// Try it with Run.
fn main() {
    let mut l = List::new();
    l.push_back(2);
    l.push_back(3);
    l.push_front(1);
    println!("{:?} len {}", l.to_vec(), l.len());
    l.reverse();
    println!("{:?} peek {:?}", l.to_vec(), l.peek());
}
