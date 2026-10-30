---
title: A Singly Linked List
summary: Build a linked list from Option<Box<Node>> with push, pop, peek, append, reverse and remove, using take() and walking with &mut cursors.
minutes: 25
difficulty: medium
tags: data-structures, ownership
id: rust-practice/linked-list
mode: harness
---
In C a linked list is a `struct node *next` chain, and half the bugs are in the pointer juggling: forgetting to update `head`, freeing a node and then reading its `next`, losing the rest of the list. Rust's ownership rules turn most of that into compile errors, but you have to learn a few moves.

## The shape

```rust
struct Node {
    value: i32,
    next: Option<Box<Node>>,
}

struct List {
    head: Option<Box<Node>>,
}
```

Each node **owns** the rest of the list through its `next`. `None` marks the end, and `Box` gives the node a fixed size. Dropping the list drops the head, which drops its `next`, and so on. No `free` anywhere.

## The key tool: take()

You cannot just move a field out of something you only borrow. `Option::take()` replaces the option with `None` and hands you the old value. It is how you detach pieces safely.

```rust
fn push_front(&mut self, value: i32) {
    let old_head = self.head.take();              // list is briefly empty
    self.head = Some(Box::new(Node { value, next: old_head }));
}

fn pop_front(&mut self) -> Option<i32> {
    self.head.take().map(|node| {
        self.head = node.next;                    // the rest becomes the list
        node.value
    })
}
```

## Peeking without taking

To look at the head without removing it, borrow instead of take. `as_ref()` turns `&Option<Box<Node>>` into `Option<&Box<Node>>`:

```rust
fn peek(&self) -> Option<&i32> {
    self.head.as_ref().map(|node| &node.value)
}
```

## Walking with a cursor

To visit every node read-only, keep a reference to the current link and step it forward:

```rust
let mut cur = &self.head;
while let Some(node) = cur {
    // use node.value
    cur = &node.next;
}
```

To **modify** the end or the middle, use a `&mut` cursor pointing at the *link* (the `Option`), not at the node. When the cursor sits on a `None` link you can overwrite it, which is how you append:

```rust
let mut cur = &mut self.head;
while cur.is_some() {
    cur = &mut cur.as_mut().unwrap().next;
}
*cur = Some(Box::new(Node { value, next: None }));
```

## Reversing

Reversal needs no allocation: pop each node off the old chain and push it onto a new one.

```rust
let mut prev = None;
let mut cur = self.head.take();
while let Some(mut node) = cur {
    cur = node.next.take();   // detach the rest
    node.next = prev;         // point back
    prev = Some(node);
}
self.head = prev;
```

## Removing from the middle

Walk the cursor until the link in front of it holds the value you want, then replace that link with the removed node's `next`.

## This lesson's exercise

Implement the methods on `List`. Duplicates are allowed; order matters.
