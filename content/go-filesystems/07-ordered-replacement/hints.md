Four fixed writes, each a `Write{Kind, Block, Value}`. Decide first what each of them targets: three of them are about `fresh` and exactly one is about `old`.
===
The inode write is the commit point. Everything the new pointer exposes (a reserved block, with its contents written) must come before it, and nothing that the old pointer still needs may be released before it.
===
The order of kinds is `alloc`, `data`, `inode`, `free`. Check it against each crash cut: after `alloc` and `data` the file is still the old one; after `inode` it is the new one; only after that may `old` be freed.
===
`Value` is `value` for the `data` write and `0` for the others, including the `inode` write. Return a fresh slice literal; do not keep a package-level slice that callers could modify.
