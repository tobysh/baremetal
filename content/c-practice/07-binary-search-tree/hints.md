Each function has the same shape: if `root == NULL`, handle that base case; otherwise do something with `root` and recurse into `root->left` and/or `root->right`.
===
`bst_contains`: if `key == root->key` return `1`; if `key < root->key` search left; else search right. `bst_height`: `1 + max(height(left), height(right))`, with `0` for `NULL`.
===
`bst_inorder`: write a helper `static void walk(const TreeNode *n, int *out, int cap, int *count)`. In it: do nothing for `NULL`; walk left; only if `*count < cap`, do `out[*count] = n->key;` and `(*count)++`; then walk right. Return `count` from the main function.
===
`bst_free`: if `root` is not `NULL`, free `root->left`, free `root->right`, then `free(root)`.
