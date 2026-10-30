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
        let old = self.head.take();
        self.head = Some(Box::new(Node { value, next: old }));
    }

    fn push_back(&mut self, value: i32) {
        let mut cur = &mut self.head;
        while cur.is_some() {
            cur = &mut cur.as_mut().unwrap().next;
        }
        *cur = Some(Box::new(Node { value, next: None }));
    }

    fn pop_front(&mut self) -> Option<i32> {
        self.head.take().map(|node| {
            self.head = node.next;
            node.value
        })
    }

    fn peek(&self) -> Option<&i32> {
        self.head.as_ref().map(|n| &n.value)
    }

    fn len(&self) -> usize {
        let mut n = 0;
        let mut cur = &self.head;
        while let Some(node) = cur {
            n += 1;
            cur = &node.next;
        }
        n
    }

    fn to_vec(&self) -> Vec<i32> {
        let mut out = Vec::new();
        let mut cur = &self.head;
        while let Some(node) = cur {
            out.push(node.value);
            cur = &node.next;
        }
        out
    }

    fn reverse(&mut self) {
        let mut prev = None;
        let mut cur = self.head.take();
        while let Some(mut node) = cur {
            cur = node.next.take();
            node.next = prev;
            prev = Some(node);
        }
        self.head = prev;
    }

    fn remove(&mut self, value: i32) -> bool {
        let mut cur = &mut self.head;
        while cur.as_ref().map_or(false, |n| n.value != value) {
            cur = &mut cur.as_mut().unwrap().next;
        }
        match cur.take() {
            Some(node) => {
                *cur = node.next;
                true
            }
            None => false,
        }
    }
}

fn main() {
    let mut l = List::new();
    l.push_back(2);
    l.push_front(1);
    println!("{:?}", l.to_vec());
}
