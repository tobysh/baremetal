---
title: Bounded Queues and Credit-Based Backpressure
summary: Stop a fast producer from drowning a slow consumer. Bound the send queue in bytes, send only while the receiver has granted credit, and fail loudly when full.
minutes: 40
difficulty: hard
tags: backpressure, flow-control, queues
id: go-protocols/backpressure
mode: harness
---
If a producer can enqueue faster than the link or the peer can drain, an unbounded queue turns a slow consumer into an out-of-memory crash. Backpressure means the slowness is passed **back** to the producer instead of being absorbed by memory.

Two mechanisms, both used by real protocols (HTTP/2 and QUIC use credit windows, TCP uses a window too):

1. **A bounded queue.** `Send` refuses with `ErrQueueFull` when accepting a frame would push the queued *bytes* over the cap. The refusal changes nothing, so the producer can retry later or drop. Count bytes, not items: one 1 KB frame is not one 10-byte frame.
2. **Credit.** The receiver may only be sent `window` unacknowledged frames at a time. `Next` hands out a frame to put on the wire only while credit remains. The peer returns credit with `Ack(n)`. A frame leaves the queue (freeing space) when `Next` releases it, and counts as in flight until acked.

Order is FIFO. Acking more than is in flight is a bug in the peer: `ErrBadAck`, with no state change.

## What this does not do

It is a single-threaded model. A real sender also needs a lock or a channel, and a timeout policy so a stalled peer does not block a producer forever. Here the caller decides what to do on `ErrQueueFull`.
