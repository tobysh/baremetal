1. **`int get_invoice(const Shop *shop, Caller who, int id, char *out, size_t cap)`** returns `200` and copies the invoice text into `out` (truncated to `cap`, terminated; `cap` 0 writes nothing) if the invoice exists **and** (`who.uid == owner` **or** `who.is_admin`). In every other case return `404` and leave `out` untouched.
2. **`long flag_enumeration(const AccessRec *log, int n, int uid)`** looks at the records of `uid` only (log is in time order, `ts` in seconds). Return the `ts` of the first record that makes **5 or more** records with `status == 404` for that user inside one window (`t - 60 < ts <= t`, up to and including that record). If it never happens return `-1`.

Different shops, users and logs are tried.
