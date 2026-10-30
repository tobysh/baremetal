Keep one table: `const TABLE: [(&str, BinOp); 3] = [("add", add), ("max", max), ("mul", mul)];` Function items coerce to pointers inside the array.
===
`lookup`: `TABLE.iter().find(|(n, _)| *n == name).map(|(_, f)| *f)`. `names`: `TABLE.iter().map(|(n, _)| *n).collect()`.
===
`fold_with`: `xs.iter().fold(init, |acc, &x| op(acc, x))`. Calling through a pointer looks exactly like a normal call.
