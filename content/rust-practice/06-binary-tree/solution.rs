struct Node {
    value: i32,
    left: Option<Box<Node>>,
    right: Option<Box<Node>>,
}

struct Tree {
    root: Option<Box<Node>>,
}

fn insert_into(slot: &mut Option<Box<Node>>, value: i32) -> bool {
    match slot {
        None => {
            *slot = Some(Box::new(Node { value, left: None, right: None }));
            true
        }
        Some(node) => {
            if value < node.value {
                insert_into(&mut node.left, value)
            } else if value > node.value {
                insert_into(&mut node.right, value)
            } else {
                false
            }
        }
    }
}

fn search(slot: &Option<Box<Node>>, value: i32) -> bool {
    match slot {
        None => false,
        Some(n) => {
            if value == n.value {
                true
            } else if value < n.value {
                search(&n.left, value)
            } else {
                search(&n.right, value)
            }
        }
    }
}

fn walk(slot: &Option<Box<Node>>, out: &mut Vec<i32>) {
    if let Some(n) = slot {
        walk(&n.left, out);
        out.push(n.value);
        walk(&n.right, out);
    }
}

fn depth(slot: &Option<Box<Node>>) -> usize {
    match slot {
        None => 0,
        Some(n) => 1 + depth(&n.left).max(depth(&n.right)),
    }
}

impl Tree {
    fn new() -> Tree {
        Tree { root: None }
    }

    fn insert(&mut self, value: i32) -> bool {
        insert_into(&mut self.root, value)
    }

    fn contains(&self, value: i32) -> bool {
        search(&self.root, value)
    }

    fn in_order(&self) -> Vec<i32> {
        let mut out = Vec::new();
        walk(&self.root, &mut out);
        out
    }

    fn height(&self) -> usize {
        depth(&self.root)
    }

    fn min(&self) -> Option<i32> {
        let mut cur = self.root.as_ref()?;
        while let Some(l) = cur.left.as_ref() {
            cur = l;
        }
        Some(cur.value)
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
