Write the early denials first: NULL user, role out of range, action out of range. After those, every `return 1` should be a case that a rule explicitly allows, and the last line of the function should be `return 0`.
===
Handle `ACT_CREATE` before looking at `d`, because it has no document. Then reject `d == NULL`. Public reads come before the organisation check, but nothing else does.
===
Compute `owner = u->uid == d->owner` once, then use a `switch` on the action. `filter_readable` is a loop that calls `authorize(u, ACT_READ, &docs[i])`.
