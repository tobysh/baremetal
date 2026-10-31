---
title: Surviving Malformed Input
summary: Replace the strict parser with a scanner that reports damage, resynchronises on the next frame, never panics, and never buffers more than one frame.
minutes: 45
difficulty: hard
tags: robustness, resync, fuzzing
id: go-protocols/malformed-input
mode: harness
---
The strict parser of lesson 2 gives up on the first bad byte. A long-lived connection that carries one flipped bit should lose one message, not the whole session. The price is a new set of obligations. A `Scanner` must:

1. **Report** each damaged region (so it can be logged and counted) without stopping.
2. **Resynchronise**: after a failure, throw away bytes until the next `Magic` byte and try again. Retry from one byte *after* the failed attempt, not after the bytes it claimed. A false header with a length of 900 would otherwise swallow the real frames that follow it.
3. **Never panic** and never index past the buffer, whatever the bytes are.
4. **Stay bounded**: after `Feed`, the buffer holds less than one complete frame (`HeaderLen+MaxPayload+TrailerLen` bytes).
5. **Give the same answer for every chunking.** To make that checkable, errors are reported once per *bad region*: after the first error the scanner is "skipping" and stays quiet until it decodes a good frame again.

At end of stream `Close()` processes what is left. A partial frame at that point is reported as `ErrTruncated`, then resynchronisation continues, because a real frame may sit inside the bytes of a false header.

## Failure investigation

The hidden tests include seeded random mutations of a valid stream (flips, deletions, insertions, truncation) and assert invariants rather than exact output: every frame returned must be a real frame found in the damaged stream, bytes are accounted for (`fed = frames + Dropped + Buffered`) and all chunkings agree.

Honest limits: resynchronising on a single magic byte can occasionally lock onto a fake frame whose CRC happens to match (probability about 2^-32 per attempt for random bytes). A CRC is not a defence against someone crafting a fake frame on purpose.
