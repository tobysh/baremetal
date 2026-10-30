In `channel_seal`, check the limit and the length first, then `nonce = ch->counter++`. Copy the plaintext into `out->ct`, call `toy_xor` on that copy and compute the tag from the ciphertext, the nonce and the caller's AAD.
===
In `channel_open`, compute `toy_tag` from the packet's nonce, the AAD and the stored ciphertext, and compare it with `p->tag`. Return before touching `out` if they differ. Decrypt by copying the ciphertext into `out` and calling `toy_xor` on it.
===
For the audit, loop over the log. For a seal, look back at earlier seals only for the same `key` and `nonce`. For each call test the kinds in order 1, 2, 3 and stop at the first call where any applies.
