**Challenge: remove and rotate a list.**

The starter has the lesson's `Node`, `Push` and `Reverse`. Add two functions that **relink the existing nodes** (no new `Node` may be allocated):

- `RemoveAll(head *Node, v int) *Node` removes every node whose `Value` is `v` and returns the new head. The list can start with the value, end with it, be made only of it, or be empty. Removed nodes should have their `Next` set to `nil` so they do not keep the rest of the list alive. A neat way to avoid special-casing the head is to walk with a `**Node` that points at the link you might need to change.
- `Rotate(head *Node, k int) *Node` moves the **last `k` nodes to the front**, keeping their order, and returns the new head. `[1 2 3 4 5]` rotated by `2` is `[4 5 1 2 3]`. `k` can be `0`, bigger than the list length (then it wraps around: use `k % length`), and the list can be empty or have one node.

Do not leave a cycle behind: the last node of the result must have `Next == nil`.
