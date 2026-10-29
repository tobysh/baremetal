You need the key length once: `size_t klen = strlen(key);` (include `<string.h>`). Then loop `for (size_t i = 0; message[i] != '\0'; i++)`.
===
The key byte for position `i` is `key[i % klen]`. The cipher byte is `(unsigned char)(message[i] ^ key[i % klen])`.
===
Print each byte with `printf("%02x", c);` and a single `printf("\n");` after the loop.
