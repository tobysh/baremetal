#include <stdio.h>
#include <stdlib.h>

typedef struct TreeNode {
    int key;
    struct TreeNode *left;
    struct TreeNode *right;
} TreeNode;

TreeNode *bst_insert(TreeNode *root, int key) {
    return root;
}

int bst_contains(const TreeNode *root, int key) {
    return 0;
}

int bst_height(const TreeNode *root) {
    return 0;
}

int bst_inorder(const TreeNode *root, int *out, int cap) {
    return 0;
}

void bst_free(TreeNode *root) {
}

// Optional: try them with Run.
int main(void) {
    TreeNode *root = NULL;
    root = bst_insert(root, 8);
    root = bst_insert(root, 3);
    printf("contains 3: %d\n", bst_contains(root, 3));
    bst_free(root);
    return 0;
}
