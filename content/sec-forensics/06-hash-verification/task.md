Keep `typedef struct {const char *name; const uint8_t *data; size_t n;} Item;`.

Implement `uint64_t fnv1a64(const uint8_t *p, size_t n)`: start at `0xcbf29ce484222325`; for each byte XOR it into the state, then multiply by `0x100000001b3`, all in `uint64_t`. The empty input hashes to the starting value.

Implement `int audit(const char *manifest, const Item *items, int n, int *status)`.

- `manifest` is zero-terminated text of lines (separated by `\n`, the last line may lack one). Each line is exactly sixteen hex digits, two spaces and a filename without spaces, at most 255 characters. Lines are valid and filenames are unique.
- For each `items[i]` (i from 0 to n-1) set `status[i]`: 2 if no manifest line has a filename exactly equal to `items[i].name` (case-sensitive, whole name); otherwise 0 if `fnv1a64(items[i].data, items[i].n)` equals the number on that line, or 1 if it does not.
- Manifest order and item order are unrelated. Two supplied items may have the same name; each is judged on its own.
- Write only `status[0..n-1]`. Return how many statuses are nonzero. `n` may be 0.

The hidden tests use manifests that are reordered, lack a final newline, contain names that are prefixes or case variants of each other, and use empty and zero-filled items. Keep the supplied type and function names.
