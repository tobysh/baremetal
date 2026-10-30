Write three functions. Different inputs are tried, including empty and one-element cases.

- `char *repeat_str(const char *s, int times)` returns a new heap string made of `s` repeated `times` times (`times` is 0 or more). `repeat_str("ab", 3)` is `"ababab"`, and `times == 0` gives the empty string `""` (still a valid heap block). Return `NULL` if allocation fails.
- `int *evens(const int *arr, size_t n, size_t *out_count)` returns a new heap array holding only the even values of `arr`, in their original order, and stores how many there are in `*out_count`. If there are none, return a valid heap block (for example of size 1) and set `*out_count` to 0. Return `NULL` on allocation failure, with `*out_count` set to 0.
- `void min_max(const int *arr, size_t n, int *min, int *max)` stores the smallest and largest of the `n` values (`n` is at least 1) in `*min` and `*max`.
