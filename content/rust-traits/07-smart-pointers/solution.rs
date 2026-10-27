use std::cell::RefCell;
use std::rc::Rc;

type NodeRef = Rc<RefCell<Node>>;

struct Node {
    value: i32,
    children: Vec<NodeRef>,
}

fn new_node(value: i32) -> NodeRef {
    Rc::new(RefCell::new(Node { value, children: Vec::new() }))
}

fn add_child(parent: &NodeRef, child: &NodeRef) {
    parent.borrow_mut().children.push(Rc::clone(child));
}

fn sum(node: &NodeRef) -> i32 {
    let n = node.borrow();
    let mut total = n.value;
    for c in &n.children {
        total += sum(c);
    }
    total
}

fn double_all(node: &NodeRef) {
    let mut n = node.borrow_mut();
    n.value *= 2;
    for c in &n.children {
        double_all(c);
    }
}

fn owners(node: &NodeRef) -> usize {
    Rc::strong_count(node)
}

fn main() {
    let root = new_node(1);
    let kid = new_node(2);
    add_child(&root, &kid);
    println!("sum = {}", sum(&root));
    double_all(&root);
    println!("kid is now {}", kid.borrow().value);
    println!("owners of kid = {}", owners(&kid));
}
