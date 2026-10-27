use std::cell::RefCell;
use std::rc::Rc;

type NodeRef = Rc<RefCell<Node>>;

struct Node {
    value: i32,
    children: Vec<NodeRef>,
}

fn new_node(value: i32) -> NodeRef {
    Rc::new(RefCell::new(Node { value: 0, children: Vec::new() }))
}

fn add_child(parent: &NodeRef, child: &NodeRef) {
}

fn sum(node: &NodeRef) -> i32 {
    0
}

fn double_all(node: &NodeRef) {
}

fn owners(node: &NodeRef) -> usize {
    0
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
