Two pieces of state, kept apart: the file's bytes (shared by everyone) and each descriptor's offset (private to that descriptor). A read looks at the shared bytes starting at *its own* offset and then moves only its own offset.
===
Every function that takes an `fd` starts with the same check: `fd` is between 0 and 3 **and** that slot is open. A small helper keeps the four uses consistent. `file_open` scans from slot 0 upwards and must reset the offset when it hands a slot out again.
===
In `file_read`, the bytes left are `32 - offset`, which cannot underflow because the offset never passes 32. If `n` is larger, shrink it. Copy `n` bytes from `file_data + offset`, add `n` to the offset, and return `n`. At end of file this naturally gives 0.
===
The same clamp-copy-advance shape on a different cursor, a reader walking through a fixed message:

```c
size_t left = MSG_LEN - cur->pos;
if (want > left) want = left;
memcpy(out, msg + cur->pos, want);
cur->pos += want;
return want;
```
