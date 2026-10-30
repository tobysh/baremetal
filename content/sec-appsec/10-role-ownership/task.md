Implement the two functions. The types and enums are in the starter. `User.role` is an `int` that should be one of the `Role` values but may hold anything. Apply these rules **in order** and stop at the first that decides:

**`int authorize(const User *u, int action, const Doc *d)`** returns 1 to allow and 0 to deny.

1. `u` is NULL, the role is outside `ROLE_GUEST..ROLE_ADMIN`, or the action is outside `ACT_READ..ACT_CREATE`: deny.
2. `ACT_CREATE`: allow only if `d` is NULL and the role is member or above.
3. `d` is NULL (any other action): deny.
4. `ACT_READ` of a public document (`is_public`): allow, whatever the role or organisation.
5. The user's `org` differs from the document's `org`: deny.
6. The document is `locked` and the action is not `ACT_READ`: deny.
7. Role is guest: deny.
8. `ACT_READ`: allow. `ACT_EDIT`: allow if the user owns the document (`uid == owner`) or is editor or admin. `ACT_DELETE` and `ACT_SHARE`: allow if the user owns it or is admin.
9. Anything else: deny.

**`size_t filter_readable(const User *u, const Doc *docs, size_t n, size_t *out_idx)`** stores the indexes (in order) of the documents `u` may read in `out_idx` and returns how many. Use `authorize`.

Every combination of role, action, owner, organisation and flags is tried, plus invalid values.
