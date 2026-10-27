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
        self.items.pop()
    }

    fn peek(&self) -> Option<&T> {
        self.items.last()
    }

    fn len(&self) -> usize {
        self.items.len()
    }

    fn is_empty(&self) -> bool {
        self.items.is_empty()
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
        if self.outbox.is_empty() {
            while let Some(x) = self.inbox.pop() {
                self.outbox.push(x);
            }
        }
        self.outbox.pop()
    }

    fn len(&self) -> usize {
        self.inbox.len() + self.outbox.len()
    }

    fn is_empty(&self) -> bool {
        self.len() == 0
    }
}

fn balanced(s: &str) -> bool {
    let mut stack: Stack<char> = Stack::new();
    for c in s.chars() {
        match c {
            '(' | '[' | '{' => stack.push(c),
            ')' | ']' | '}' => {
                let want = match c {
                    ')' => '(',
                    ']' => '[',
                    _ => '{',
                };
                if stack.pop() != Some(want) {
                    return false;
                }
            }
            _ => {}
        }
    }
    stack.is_empty()
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
