---
title: Binary Search Trees
summary: Grow a tree of malloc'd nodes where smaller keys go left and bigger go right, then search it, measure it, list it in order and free it.
minutes: 45
difficulty: hard
tags: data-structures, recursion, heap
id: c-practice/binary-search-tree
mode: harness
---
A linked list has one way forward. A **tree** has branches. Each node holds a value and up to two children, **left** and **right**. The first node is the **root**, and a node with no children is a **leaf**.

A **binary search tree** (BST) adds one rule that makes searching fast:

> Everything in a node's left branch is **smaller** than the node. Everything in its right branch is **bigger**.

```
        8
      /   \
     3     10
    / \      \
   1   6      14
```

To find `6`: start at `8`; `6 < 8`, so go left to `3`; `6 > 3`, so go right; found. Each step throws away half of the remaining tree (when the tree is balanced), so searching a million keys takes about twenty steps.

```c
typedef struct TreeNode {
    int key;
    struct TreeNode *left;
    struct TreeNode *right;
} TreeNode;
```

An empty tree is simply `NULL`.

## Trees love recursion

A tree is made of smaller trees: the left child *is* the root of a smaller tree. So most operations are short recursive functions with the base case "this subtree is `NULL`".

### Insert

Insert returns the (possibly new) root of the subtree. An empty subtree gets a fresh node, otherwise go left or right and hook up the result:

```c
TreeNode *bst_insert(TreeNode *root, int key) {
    if (root == NULL) {
        // malloc a node, set key, left = right = NULL, return it
    }
    if (key < root->key)      root->left  = bst_insert(root->left, key);
    else if (key > root->key) root->right = bst_insert(root->right, key);
    return root;              // equal keys: already there, change nothing
}
```

The caller writes `root = bst_insert(root, 5);`. That is how the very first insert makes a root.

### Height

The **height** is the number of nodes on the longest path from the root down to a leaf. An empty tree has height `0`, a single node `1`. The height of a node is `1` plus the taller of its two subtrees. A tree built from sorted input (`1, 2, 3, 4...`) is just a list leaning to the right, with height `n`. That is the weakness of a plain BST, and why balanced trees were invented.

### In-order walk

Visit the left subtree, then the node, then the right subtree. For a BST this lists the keys **in sorted order**, which is a beautiful property. You can write each key into an array as you go, using a counter that all the recursive calls share.

### Freeing

You must free the children *before* the node, otherwise you lose the pointers to them (post-order: left, right, then the node itself).

## Your turn

Write `bst_insert`, `bst_contains`, `bst_height`, `bst_inorder` and `bst_free`.
