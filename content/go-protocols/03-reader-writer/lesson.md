---
title: Frames over io.Reader and io.Writer
summary: Read exactly one frame from a stream that returns data in dribbles, and write a frame to a writer that may fail partway, without over-reading or interleaving.
minutes: 35
difficulty: medium
tags: io, partial-io, errors
id: go-protocols/reader-writer
mode: harness
---
Lesson 2 pushed bytes into a parser. Many programs instead *pull* from an `io.Reader`. The contract of `Read` is generous to the implementation: it may return fewer bytes than asked, may return `n > 0` **together with** an error such as `io.EOF`, and may return `0, nil`. `io.ReadFull` hides most of this by looping until the buffer is full.

## What the end of the stream means

- EOF with **zero** bytes of a frame read: the peer closed cleanly between frames. Return `io.EOF`.
- EOF in the **middle** of a frame: the stream was cut. Return `io.ErrUnexpectedEOF`. A caller must be able to tell these apart.
- Any other reader error is passed through (`errors.Is` must still match).

## Do not read more than you need

A reader that is shared (a connection, then the next protocol stage) must not lose bytes. Read the 4-byte header, validate it, and only then read exactly `len+4` more bytes. Never wrap the reader in a buffer you throw away. A bad magic or oversized length must be rejected **before** allocating or reading the payload.

## Writing

`io.Writer` promises to return a non-nil error if it wrote fewer bytes than given. Still, defend against a writer that breaks the promise: report `io.ErrShortWrite`. Write the whole frame in **one** `Write` call so two goroutines sharing a connection (with a lock around `WriteFrame`) cannot interleave halves of two frames.

Limitation: a hidden-test reader models short reads and errors, not real socket timing.
