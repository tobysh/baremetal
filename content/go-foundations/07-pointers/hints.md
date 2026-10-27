`Swap`: `*a, *b = *b, *a` works because Go evaluates the right side first. `Increment`: check `p != nil`, then `*p++`.
===
`Push` is one line: `return &Node{Value: v, Next: head}`.
===
`Reverse`: keep `var prev *Node`. Loop `for head != nil { next := head.Next; head.Next = prev; prev = head; head = next }` and return `prev`.
