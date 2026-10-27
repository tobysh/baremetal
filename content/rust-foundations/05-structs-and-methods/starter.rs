struct Stack {
    items: Vec<i32>,
}

impl Stack {
    fn new() -> Stack {
        Stack { items: Vec::new() }
    }

    fn push(&mut self, x: i32) {
    }

    fn pop(&mut self) -> Option<i32> {
        None
    }

    fn peek(&self) -> Option<i32> {
        None
    }

    fn len(&self) -> usize {
        0
    }

    fn is_empty(&self) -> bool {
        false
    }
}

// Try your stack with Run.
fn main() {
    let mut s = Stack::new();
    s.push(1);
    s.push(2);
    println!("peek: {:?}, len: {}", s.peek(), s.len());
    println!("pop: {:?}", s.pop());
}
