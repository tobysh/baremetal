#[test]
fn new_node_has_value_and_no_children() {
    let n = new_node(7);
    assert_eq!(n.borrow().value, 7, "value should be stored");
    assert_eq!(n.borrow().children.len(), 0, "a new node has no children");
}

#[test]
fn owners_starts_at_one() {
    let n = new_node(1);
    assert_eq!(owners(&n), 1, "a new node has one owner");
}

#[test]
fn add_child_shares_instead_of_copying() {
    let parent = new_node(1);
    let child = new_node(2);
    add_child(&parent, &child);
    assert_eq!(parent.borrow().children.len(), 1, "parent should have one child");
    assert_eq!(owners(&child), 2, "child is owned by its creator and by the parent");
    child.borrow_mut().value = 50;
    assert_eq!(parent.borrow().children[0].borrow().value, 50, "the parent must see changes made through the other handle");
}

#[test]
fn sum_of_single_node() {
    assert_eq!(sum(&new_node(9)), 9, "a lone node sums to its value");
}

#[test]
fn sum_of_whole_tree() {
    let root = new_node(1);
    let a = new_node(2);
    let b = new_node(3);
    let c = new_node(4);
    add_child(&root, &a);
    add_child(&root, &b);
    add_child(&a, &c);
    assert_eq!(sum(&root), 10, "1 + 2 + 3 + 4");
    assert_eq!(sum(&a), 6, "subtree a: 2 + 4");
}

#[test]
fn sum_counts_a_shared_node_once_per_parent() {
    let root = new_node(1);
    let a = new_node(10);
    let b = new_node(20);
    let shared = new_node(100);
    add_child(&root, &a);
    add_child(&root, &b);
    add_child(&a, &shared);
    add_child(&b, &shared);
    assert_eq!(owners(&shared), 3, "creator plus two parents");
    assert_eq!(sum(&root), 1 + 10 + 20 + 100 + 100, "shared node is reachable by two paths");
}

#[test]
fn double_all_changes_every_node_in_place() {
    let root = new_node(1);
    let a = new_node(2);
    let c = new_node(3);
    add_child(&root, &a);
    add_child(&a, &c);
    double_all(&root);
    assert_eq!(root.borrow().value, 2, "root doubled");
    assert_eq!(a.borrow().value, 4, "child doubled, seen through the original handle");
    assert_eq!(c.borrow().value, 6, "grandchild doubled");
    assert_eq!(sum(&root), 12, "sum after doubling");
}

#[test]
fn dropping_a_handle_lowers_the_count() {
    let parent = new_node(1);
    let child = new_node(2);
    add_child(&parent, &child);
    drop(parent);
    assert_eq!(owners(&child), 1, "once the parent is gone only the original handle remains");
}
