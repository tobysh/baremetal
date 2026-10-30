Implement two functions that return `1` when the sample is flagged and `0` otherwise:

1. `int detect_v1(const char *sample)`: exactly `strstr(sample, "NET BEACON") != NULL`.
2. `int detect_v2(const char *sample)`: the normalising, decoding rule described in the lesson (comment lines skipped; `PACK xor=KK <hex>` decoded and analysed recursively up to depth 3; delete `"` and `+`; lower-case; squeeze spaces and tabs; hit when the line is `net beacon` or begins with `net beacon `).

Samples are NUL-terminated text, at most 4000 characters, with lines separated by `\n`. Many generated samples are tried: plain, disguised and packed ones, and harmless ones that merely mention the words.
