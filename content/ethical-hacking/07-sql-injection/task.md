1. **`int find_user_ids(const DbApi *api, void *db, const char *name, int *ids, int cap)`** returns the ids of users whose name equals `name` exactly (case-sensitive), at most `cap` of them, and the number stored. It must **never** call `api->exec_text`. Use `api->exec_param(db, "SELECT id FROM users WHERE name = ?", name, ids, cap)` and return its result. If `name` is `NULL` return `0`.
2. **`int looks_like_injection(const char *input)`** returns `1` if the input contains any of: a single quote `'`, `--`, `;`, `/*`, or (ignoring case) the words ` or ` or ` union ` with a space on each side. Otherwise `0`; `NULL` gives `0`.

Hostile inputs and awkward honest names (`O'Brien`) are both tried against several databases.
