Implement three functions:

1. `int identify(const char *banner, char *product, size_t pcap, char *version, size_t vcap)` returns `PROTO_SSH`, `PROTO_SMTP`, `PROTO_FTP`, `PROTO_HTTP` or `PROTO_UNKNOWN` and fills in `product` and `version` (empty strings when absent or unknown). Formats:
   - SSH: starts `SSH-2.0-` or `SSH-1.99-`; the software token runs to the first space or line end. Split at its last `_`: `quillsh_3.2` gives product `quillsh`, version `3.2`; no `_` means a product only.
   - SMTP: starts `220 ` and has ` ESMTP` as a word. The next word is the product, the word after is the version only if it starts with a digit.
   - FTP: starts `220 (`. Inside the parentheses: product, a space, an optional version.
   - HTTP: starts `HTTP/1.`; find the line `Server: value` (name matched case-insensitively), and split value at the first `/`.
   Output buffers must never be overrun: truncate to fit, and always terminate. A capacity of 0 writes nothing.
2. `int ver_cmp(const char *a, const char *b)` returns negative, 0 or positive. Compare dotted numeric components (missing = 0, stop at the first character that is neither digit nor dot).
3. `int advisory_check(const char *product, const char *version, const Advisory *adv, int n)` returns the index of the first advisory with the same product whose `fixed_in` is newer than `version`; `-1` if none; `-2` if the product has an advisory but `version` is empty (cannot be checked).

Many generated banners are tried, including hardened ones and tiny output buffers.
