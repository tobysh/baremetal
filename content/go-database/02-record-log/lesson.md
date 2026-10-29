---
title: An Append-Only Log
summary: Write key-value records to a byte stream with length prefixes, and read them back one at a time.
minutes: 30
difficulty: medium
tags: log, binary, io
id: go-database/record-log
mode: harness
---
A map lives in memory and vanishes when the program exits. To keep data, a database writes to a file. The oldest trick is the **append-only log**: never edit anything in place, only add new records at the end. Appending is fast, and it is hard to damage old data by accident.

## Length-prefixed records

A stream of bytes has no boundaries, so each record says how long it is. Here a record is two fixed-size lengths followed by the bytes themselves:

```
+-----------+-----------+---------+-----------+
| keyLen u32| valLen u32|   key   |   value   |
+-----------+-----------+---------+-----------+
```

Integers are written **big-endian** (most significant byte first). `encoding/binary` does it for you:

```go
var hdr [8]byte
binary.BigEndian.PutUint32(hdr[0:4], uint32(len(key)))
binary.BigEndian.PutUint32(hdr[4:8], uint32(len(value)))
```

Build the whole record in one slice and call `w.Write` **once**. This reduces opportunities for separate header and body writes to interleave, but does not make a record atomic or durable. A writer can still return a short count or an error, and a crash can leave a torn tail. Propagate its error; if its count is short without an error, return `io.ErrShortWrite`. Concurrent writers would need coordination, and durable files would need an explicit synchronization policy.

## Reading it back

`io.ReadFull(r, buf)` reads exactly `len(buf)` bytes and tells you what went wrong if it cannot:

- `io.EOF` means it read **zero** bytes: the stream ended cleanly between records.
- `io.ErrUnexpectedEOF` means it read **some** bytes and then ran out: the record is cut off.

A reader should pass these on, because they mean different things. A clean `io.EOF` is the normal end; a cut-off record means the last write never finished. The next lesson depends on that difference.

One more defence: the length fields come from untrusted bytes. If a header says the key is 4 GB long, do not try to allocate it. Reject any field above a sensible limit.
