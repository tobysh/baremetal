**Track challenge: canary-aware payload and transactional record parsing.**

Combine binary layout, integer bounds and defensive parsing. The fictional frame now contains `offset` local-buffer bytes, an eight-byte canary and an eight-byte saved return target, in that order. Both integers use little-endian encoding. The canary value is supplied as known lab data; discovering a native canary is outside this model.

Keep the supplied lesson functions and add:

```c
size_t make_guarded_payload(uint8_t *out, size_t cap, size_t offset,
                            uint64_t cookie, uint64_t win);
int copy_record(uint8_t *frame, size_t offset,
                const uint8_t *record, size_t record_len);
```

`make_guarded_payload` writes padding, then the unchanged cookie, then win. Return offset+16. Return zero without writing if out is NULL, offset+16 would overflow size_t, or capacity is insufficient. Write no trailing terminator or bytes beyond the payload. The modeled dispatcher only selects win when the cookie is preserved **and** the return slot changes to win.

`copy_record` is the defence. The wire format is a two-byte unsigned little-endian length followed by exactly that many payload bytes. Require record != NULL, record_len >= 2, the declared length == record_len-2, and declared length <= offset. A nonempty payload also requires frame != NULL. On success copy only the payload into the local buffer and return one. On **any** rejection return zero without changing any byte of frame. A valid zero-length record succeeds even if frame is NULL. Caller supplies a valid nonoverlapping record allocation and, when non-NULL, a frame of at least offset+16 bytes.

Example: `{3,0,'a','b','c'}` copies three bytes if offset >= 3. `{3,0,'a','b'}` rejects as truncated. `{1,0,'a','x'}` rejects as trailing data. A payload that fits the entire frame but exceeds offset must still be rejected.

**Why this is harder:** the attacker must preserve one control value while changing another, and the defender must distinguish physical allocation capacity from the logical local-buffer limit. Binary lengths cannot be validated with strlen. Check every condition before the first write, so failure is transactional.

**What the tests establish:** multiple offsets, zero-containing cookie/target values, little-endian encoding, output guards, size_t overflow, complete/truncated/trailing records, maximum 16-bit lengths and unchanged memory after rejection. This remains a deterministic model; no address from the payload is executed.
