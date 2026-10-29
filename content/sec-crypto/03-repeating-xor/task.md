The starter declares a `message` and a `key`. Print the message encrypted with the repeating-key XOR (byte `i` is XORed with `key[i % strlen(key)]`) as one line of lowercase hex, with no spaces, followed by a newline.

For `"Burn the letters"` and key `"ICE"` the first bytes are `0b 36 37 27 ...`, printed as `0b3731...`.

Your program will also be run with other messages and keys, so keep the variables `message` and `key` and compute from them.
