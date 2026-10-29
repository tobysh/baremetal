#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

typedef struct Node {
    int key;
    struct Node *left;
    struct Node *right;
} Node;

Node *bst_insert(Node *root, int key) {
    if (root == NULL) {
        Node *n = malloc(sizeof *n);
        n->key = key;
        n->left = n->right = NULL;
        return n;
    }
    if (key < root->key) root->left = bst_insert(root->left, key);
    else if (key > root->key) root->right = bst_insert(root->right, key);
    return root;
}

void bst_free(Node *root) {
    if (root == NULL) return;
    bst_free(root->left);
    bst_free(root->right);
    free(root);
}

const Node *bst_min(const Node *root) {
    return NULL;
}

Node *bst_delete(Node *root, int key) {
    return root;
}

int bst_size(const Node *root) {
    return 0;
}

int bst_is_valid(const Node *root) {
    return 0;
}

// Optional: try it with Run.
int main(void) {
    Node *root = NULL;
    int keys[] = {8, 3, 10, 1, 6};
    for (int i = 0; i < 5; i++) root = bst_insert(root, keys[i]);
    root = bst_delete(root, 3);
    printf("size %d (expected 4)\n", bst_size(root));
    bst_free(root);
    return 0;
}
