Delete the `snprintf` and the call to `exec_text`. The parameter interface wants the template with a literal `?` and the raw name as a separate argument.
===
For `looks_like_injection`, lowercase a copy of the input (or compare case-insensitively with a small helper) and search for each pattern with `strstr`.
===
```c
int find_user_ids(const DbApi *api, void *db, const char *name, int *ids, int cap) {
    if (!name) return 0;
    return api->exec_param(db, "SELECT id FROM users WHERE name = ?", name, ids, cap);
}
```
