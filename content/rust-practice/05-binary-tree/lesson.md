---
title: A Binary Search Tree
summary: Build a binary search tree from Option<Box<Node>>, with insert, contains, in-order traversal and height, using recursion and &mut references.
minutes: 25
difficulty: medium
id: rust-practice/binary-tree
mode: harness
---
A **binary search tree** keeps numbers in a shape that makes searching quick. Every node holds a value and up to two children. Everything in the left branch is smaller than the node, everything in the right is larger. To find a value you start at the top and, at each node, go left or right. In a reasonably balanced tree, a million values take about twenty steps.

## The Rust shape

A node contains its children, so it contains nodes, which contain nodes... A type cannot contain itself directly, because it would be infinitely large. `Box` solves it: a box is a pointer to heap data, and a pointer has a fixed size. `Option` says "there may be no child".

```rust
struct Node {
    value: i32,
    left: Option<Box<Node>>,
    right: Option<Box<Node>>,
}

struct Tree {
    root: Option<Box<Node>>,
}
```

An empty tree is `root: None`. A leaf has two `None` children.

## Recursion on a slot

The neat way to write `insert` is a helper that takes **a slot**: a `&mut Option<Box<Node>>`. If the slot is empty, put a new node there. If it holds a node, compare and recurse into that node's left or right slot.

```rust
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
                false // already present
            }
        }
    }
}
```

Because the slot is borrowed mutably, you can overwrite it in place: no special "parent pointer" juggling, which is what makes trees painful in other languages.

Read-only operations (`contains`, traversal) take `&Option<Box<Node>>` and recurse the same way. `node` in `Some(node)` is a `&Box<Node>`, and you can use `node.value` and `node.left` directly because `Box` dereferences automatically.

## In-order traversal

Visit the left branch, then the node, then the right branch. For a search tree that yields the values in sorted order, which is a lovely way to check your work.

## This lesson's exercise

Implement `insert`, `contains`, `in_order`, `height` and `min` for `Tree`. Duplicates are ignored.
