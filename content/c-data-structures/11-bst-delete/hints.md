`bst_min` is a loop: while `root->left` is not `NULL`, move left. `bst_size` is `1 + size(left) + size(right)`.
===
For `bst_delete`, recurse left or right until you find the key. With zero or one child, free the node and return the other child (which is `NULL` for a leaf). With two children, find `bst_min(root->right)`, copy its key into `root->key`, then `root->right = bst_delete(root->right, that_key)`.
===
For `bst_is_valid`, write a helper `check(root, long long lo, long long hi)` that returns 0 if `root->key <= lo` or `>= hi`, and otherwise checks `check(root->left, lo, root->key)` and `check(root->right, root->key, hi)`. Start with `LLONG_MIN` and `LLONG_MAX` (include `<limits.h>`).
