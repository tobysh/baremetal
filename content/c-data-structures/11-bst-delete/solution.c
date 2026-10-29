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
    if (root == NULL) return NULL;
    while (root->left) root = root->left;
    return root;
}

Node *bst_delete(Node *root, int key) {
    if (root == NULL) return NULL;
    if (key < root->key) {
        root->left = bst_delete(root->left, key);
    } else if (key > root->key) {
        root->right = bst_delete(root->right, key);
    } else {
        if (root->left == NULL) {
            Node *r = root->right;
            free(root);
            return r;
        }
        if (root->right == NULL) {
            Node *l = root->left;
            free(root);
            return l;
        }
        int succ = bst_min(root->right)->key;
        root->key = succ;
        root->right = bst_delete(root->right, succ);
    }
    return root;
}

int bst_size(const Node *root) {
    if (root == NULL) return 0;
    return 1 + bst_size(root->left) + bst_size(root->right);
}

static int check(const Node *n, long long lo, long long hi) {
    if (n == NULL) return 1;
    if (n->key <= lo || n->key >= hi) return 0;
    return check(n->left, lo, n->key) && check(n->right, n->key, hi);
}

int bst_is_valid(const Node *root) {
    return check(root, LLONG_MIN, LLONG_MAX);
}

int main(void) {
    Node *root = NULL;
    int keys[] = {8, 3, 10, 1, 6};
    for (int i = 0; i < 5; i++) root = bst_insert(root, keys[i]);
    root = bst_delete(root, 3);
    printf("size %d (expected 4)\n", bst_size(root));
    bst_free(root);
    return 0;
}
