#include <stdio.h>
#include <stdlib.h>

typedef struct TreeNode {
    int key;
    struct TreeNode *left;
    struct TreeNode *right;
} TreeNode;

TreeNode *bst_insert(TreeNode *root, int key) {
    if (root == NULL) {
        TreeNode *n = malloc(sizeof(TreeNode));
        if (n == NULL) {
            return NULL;
        }
        n->key = key;
        n->left = NULL;
        n->right = NULL;
        return n;
    }
    if (key < root->key) {
        root->left = bst_insert(root->left, key);
    } else if (key > root->key) {
        root->right = bst_insert(root->right, key);
    }
    return root;
}

int bst_contains(const TreeNode *root, int key) {
    if (root == NULL) {
        return 0;
    }
    if (key == root->key) {
        return 1;
    }
    if (key < root->key) {
        return bst_contains(root->left, key);
    }
    return bst_contains(root->right, key);
}

int bst_height(const TreeNode *root) {
    if (root == NULL) {
        return 0;
    }
    int l = bst_height(root->left);
    int r = bst_height(root->right);
    return 1 + (l > r ? l : r);
}

static void walk(const TreeNode *n, int *out, int cap, int *count) {
    if (n == NULL) {
        return;
    }
    walk(n->left, out, cap, count);
    if (*count < cap) {
        out[*count] = n->key;
        (*count)++;
    }
    walk(n->right, out, cap, count);
}

int bst_inorder(const TreeNode *root, int *out, int cap) {
    int count = 0;
    walk(root, out, cap, &count);
    return count;
}

void bst_free(TreeNode *root) {
    if (root == NULL) {
        return;
    }
    bst_free(root->left);
    bst_free(root->right);
    free(root);
}

int main(void) {
    TreeNode *root = NULL;
    root = bst_insert(root, 8);
    root = bst_insert(root, 3);
    printf("contains 3: %d\n", bst_contains(root, 3));
    bst_free(root);
    return 0;
}
