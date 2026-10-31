Write the class choice as an `if / else if / else` chain that yields the three relevant bits, then compare with `want`.
===
Once the owner matches, do not fall through to the group. `0070` means the owner is refused.
===
For root, check execute separately: `(want & P_X) && !is_dir && !(mode & 0111)` is the only way root is refused.
