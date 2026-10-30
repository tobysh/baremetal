struct Node {
    value: i32,
    left: Option<Box<Node>>,
    right: Option<Box<Node>>,
}

struct Tree {
    root: Option<Box<Node>>,
}

impl Tree {
    fn new() -> Tree {
        Tree { root: None }
    }

    fn insert(&mut self, value: i32) -> bool {
        false
    }

    fn contains(&self, value: i32) -> bool {
        false
    }

    fn in_order(&self) -> Vec<i32> {
        Vec::new()
    }

    fn height(&self) -> usize {
        0
    }

    fn min(&self) -> Option<i32> {
        None
    }
}

// Try it with Run.
fn main() {
    let mut t = Tree::new();
    for v in [5, 2, 8, 1, 9, 3] {
        t.insert(v);
    }
    println!("{:?} height {}", t.in_order(), t.height());
    println!("{} {:?}", t.contains(8), t.min());
}
