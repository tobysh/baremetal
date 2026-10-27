struct Stack<T> {
    items: Vec<T>,
}

impl<T> Stack<T> {
    fn new() -> Stack<T> {
        Stack { items: Vec::new() }
    }

    fn push(&mut self, item: T) {
        self.items.push(item);
    }

    fn pop(&mut self) -> Option<T> {
        None
    }

    fn peek(&self) -> Option<&T> {
        None
    }

    fn len(&self) -> usize {
        0
    }

    fn is_empty(&self) -> bool {
        true
    }
}

struct Queue<T> {
    inbox: Vec<T>,
    outbox: Vec<T>,
}

impl<T> Queue<T> {
    fn new() -> Queue<T> {
        Queue { inbox: Vec::new(), outbox: Vec::new() }
    }

    fn enqueue(&mut self, item: T) {
        self.inbox.push(item);
    }

    fn dequeue(&mut self) -> Option<T> {
        None
    }

    fn len(&self) -> usize {
        0
    }

    fn is_empty(&self) -> bool {
        true
    }
}

fn balanced(s: &str) -> bool {
    false
}

// Try it with Run.
fn main() {
    let mut s = Stack::new();
    s.push(1);
    s.push(2);
    println!("{:?}", s.peek());
    println!("{:?}", s.pop());
    let mut q = Queue::new();
    q.enqueue("a");
    q.enqueue("b");
    println!("{:?}", q.dequeue());
    println!("{}", balanced("{[()]}"));
}
