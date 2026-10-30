---
title: Short and Truncated Writes
summary: A Write may accept fewer bytes than you offered and still report no error. Loop until everything is out, stop on real errors, and read back truncated data correctly.
minutes: 30
difficulty: medium
tags: io, short-write, fault-injection
id: go-fault-injection/short-writes
mode: harness
---
`io.Writer` has a contract that surprises people: `Write(p)` returns `n` bytes accepted, and `n < len(p)` is allowed. A conforming writer must then return a non-nil error, but real writers (pipes, sockets, wrappers around them) sometimes accept part of the data and report success, and callers are expected to carry on with the rest. Code that calls `Write` once and moves on silently drops bytes.

## Injecting the failure

You cannot wait for a pipe to misbehave, so you build a writer that does it on a schedule. The hidden tests use a writer like this:

```go
type scripted struct {
    sink   bytes.Buffer
    chunks []int // max bytes accepted per call, in order; the last entry repeats
    failAt int   // after this many total bytes, return an error (-1 = never)
    calls  int
}
```

A schedule such as `chunks = [3, 0, 2, 100]` means: accept 3 bytes, then none, then 2, then everything. `failAt = 7` means: write 7 bytes in total, then fail. The test then checks an **invariant** that holds for every schedule: whatever the sink holds is exactly a prefix of the payload (nothing duplicated, nothing reordered), and on success it is the whole payload.

## Three rules for `WriteFull`

1. Keep writing the remainder until every byte is accepted. Never resend bytes that were accepted.
2. If `Write` returns an error, stop at once and return the total accepted so far with that error. The bytes already out cannot be taken back.
3. A writer that accepts nothing and reports no error could loop forever. After **3 consecutive calls that accept zero bytes**, give up with `io.ErrShortWrite`.

## Truncation on the read side

The same failures leave truncated data behind: a crash mid-write leaves half a frame in a file. A reader must tell apart "the stream ended cleanly between frames" (`io.EOF`) from "the stream ended inside a frame" (`io.ErrUnexpectedEOF`). `io.ReadFull` helps, but it returns plain `io.EOF` when zero bytes were read, which is wrong when the header was already consumed and the body is missing.

The frame format is a 2-byte big-endian length followed by that many payload bytes.

## Limits of this model

The scripted writer models accepted-byte counts and an error point. It does not model data that was accepted but never reached the disk; that comes in the atomic update lesson.
