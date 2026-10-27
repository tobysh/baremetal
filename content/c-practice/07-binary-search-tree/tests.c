int main(void) {
    TreeNode *root = NULL;
    CHECK("an empty tree contains nothing", bst_contains(root, 5) == 0, "expected 0");
    CHECK("an empty tree has height 0", bst_height(root) == 0, "got %d", bst_height(root));
    int buf[16];
    CHECK("an empty tree has no keys to list", bst_inorder(root, buf, 16) == 0, "expected 0");
    bst_free(root);

    int keys[] = {8, 3, 10, 1, 6, 14};
    for (int i = 0; i < 6; i++) root = bst_insert(root, keys[i]);
    CHECK("the first insert makes a root", root != NULL && root->key == 8, "root is wrong");
    CHECK("smaller keys go left, larger go right",
          root != NULL && root->left != NULL && root->right != NULL && root->left->key == 3 && root->right->key == 10 && root->left->left != NULL && root->left->left->key == 1 && root->left->right != NULL && root->left->right->key == 6 && root->right->right != NULL && root->right->right->key == 14 && root->right->left == NULL,
          "the tree does not have the expected shape");

    CHECK("bst_contains finds every inserted key", bst_contains(root, 8) && bst_contains(root, 1) && bst_contains(root, 6) && bst_contains(root, 14), "a key was not found");
    CHECK("bst_contains rejects keys that are absent", !bst_contains(root, 7) && !bst_contains(root, 0) && !bst_contains(root, 100), "found a key that is not there");

    CHECK("height of the example tree is 3", bst_height(root) == 3, "got %d", bst_height(root));

    int n = bst_inorder(root, buf, 16);
    int sorted[] = {1, 3, 6, 8, 10, 14};
    int ok = n == 6;
    for (int i = 0; ok && i < 6; i++) if (buf[i] != sorted[i]) ok = 0;
    CHECK("bst_inorder lists keys in ascending order", ok, "returned %d: %d %d %d %d %d %d", n, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);

    int small[3] = {-1, -1, -1};
    int canary[1] = {77};
    n = bst_inorder(root, small, 3);
    CHECK("bst_inorder writes at most cap keys", n == 3 && small[0] == 1 && small[1] == 3 && small[2] == 6 && canary[0] == 77, "returned %d: %d %d %d", n, small[0], small[1], small[2]);

    int before = bst_height(root);
    root = bst_insert(root, 6);
    root = bst_insert(root, 8);
    n = bst_inorder(root, buf, 16);
    CHECK("duplicate keys are ignored", n == 6 && bst_height(root) == before, "list has %d keys", n);
    bst_free(root);

    TreeNode *line = NULL;
    for (int i = 1; i <= 100; i++) line = bst_insert(line, i);
    CHECK("keys inserted in sorted order make a tall tree (height 100)", bst_height(line) == 100, "got %d", bst_height(line));
    CHECK("searching the tall tree still works", bst_contains(line, 100) && bst_contains(line, 1) && !bst_contains(line, 101), "search failed");
    bst_free(line);

    TreeNode *rnd = NULL;
    unsigned int seed = 99u;
    int all[200];
    int count = 0;
    for (int i = 0; i < 200; i++) {
        seed = seed * 1103515245u + 12345u;
        int k = (int)((seed >> 16) % 1000u) - 500;
        rnd = bst_insert(rnd, k);
        int dup = 0;
        for (int j = 0; j < count; j++) if (all[j] == k) dup = 1;
        if (!dup) all[count++] = k;
    }
    int out[200];
    n = bst_inorder(rnd, out, 200);
    ok = n == count;
    for (int i = 0; ok && i + 1 < n; i++) if (out[i] >= out[i + 1]) ok = 0;
    for (int i = 0; ok && i < count; i++) if (!bst_contains(rnd, all[i])) ok = 0;
    CHECK("200 pseudo-random inserts: sorted, no duplicates, nothing lost", ok, "listed %d keys, expected %d", n, count);
    bst_free(rnd);
    CHECK("bst_free accepts NULL", (bst_free(NULL), 1), "crashed");
    return 0;
}
