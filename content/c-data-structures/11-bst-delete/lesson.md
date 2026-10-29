---
title: Binary Search Tree Deletion
summary: Finish the binary search tree. Find the minimum, delete a node in all three cases, and check the search-tree property properly with value bounds.
minutes: 45
difficulty: hard
tags: trees, recursion, pointers, data-structures
id: c-data-structures/bst-delete
mode: harness
---
In [C in Practice](/learn/c-practice/binary-search-tree) you built a binary search tree with insert, search, height and listing. Insertion is the easy half. This lesson adds the hard half: **deleting** a key, and **validating** that a tree really is a search tree. Insert and free are provided.

The **search-tree property**: for every node, every key in its left subtree is smaller and every key in its right subtree is larger.

## Minimum

The smallest key is found by following `left` until there is no left child.

## Deleting a key

Find the node (go left or right by comparison), then there are three cases:

1. **Leaf** (no children): free it; its parent now points to `NULL`.
2. **One child**: free it; its parent points to that child instead.
3. **Two children**: you cannot simply remove it. Copy the **in-order successor** (the minimum of the right subtree) into this node, then delete that successor from the right subtree. The successor has no left child, so its removal is case 1 or 2.

A recursive function that *returns the new root of the subtree* handles all cases neatly, because the caller just stores the result back in its child pointer:

```c
Node *bst_delete(Node *root, int key) {
    if (root == NULL) return NULL;                 // key not present
    if (key < root->key)      root->left  = bst_delete(root->left,  key);
    else if (key > root->key) root->right = bst_delete(root->right, key);
    else {
        if (root->left == NULL)  { Node *r = root->right; free(root); return r; }
        if (root->right == NULL) { Node *l = root->left;  free(root); return l; }
        // two children: ...
    }
    return root;
}
```

## Validating a BST

Checking only "left child < node < right child" is **not enough**. In this tree, every local check passes, but 11 sits in the left subtree of 10, so it is not a search tree:

```text
      10
     /  \
    5    15
     \
      11
```

The correct check carries an open interval `(lo, hi)` down the recursion: the root may hold any key, a left child must be below its parent's key (and still above the inherited `lo`), and a right child must be above it (and still below the inherited `hi`). Use `long long` bounds so that `INT_MIN` and `INT_MAX` keys work.

## Common mistakes

- Freeing the node and then reading from it.
- Forgetting to store the result of the recursive call back into the child pointer.
- Deleting the successor with the wrong key or from the wrong subtree.
- Validating only parent-child pairs.

Implement the four functions.
