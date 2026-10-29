Keep an array of groups. Find the group by comparing key and lowercased nonce; add a message id only if it is not already in the group.
===
Collect the groups with two or more ids into an index array and sort it by `strcmp` on key, then nonce, before printing.
