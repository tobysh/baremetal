---
title: "Capstone: Client and Server over a Faulty Pipe"
summary: Combine framing, negotiation and flow control over a simulated link that splits, drops, corrupts and duplicates bytes.
minutes: 75
difficulty: hard
tags: capstone, protocol, retransmission
id: go-protocols/capstone-exchange
mode: harness
---
You now have every part: the codec (lesson 1), a resynchronising `Scanner` (lessons 2 and 6), `Hello`/`Negotiate` (lesson 4) and a credit window (lesson 5). This lesson assembles them into a small request/response protocol.

## The conversation

```
client                                   server
  HELLO(min,max,features,required) -->
                         <-- WELCOME(version, features)     or ERR(text)
  REQ(seq=1, body) --> ... up to Window requests unanswered at once
                         <-- RESP(seq=1, reversed body)
  BYE -->                                   (after every request is answered)
```

Sequence numbers start at 1 and equal request index + 1. The server's job is a pure function (reverse the body), so answering a duplicate request again is harmless: the protocol relies on **idempotent** requests, which is what makes blind retransmission safe.

## The simulated link

The hidden test harness is a deterministic, single-threaded simulation with a fake clock measured in ticks. Each write by either side is passed through a seeded fault pipe that may **drop** it, **corrupt** one byte, **duplicate** it, and then **split** it into chunks of random size. Order is preserved and delivery takes one tick. There are no goroutines and no wall-clock time, so a failure is reproducible from its seed.

Client behaviour:
- `Tick(0)` sends HELLO. Without a WELCOME it resends every `RetryTicks` ticks.
- After WELCOME it keeps at most `Window` distinct requests unanswered, retransmits an unanswered request after `RetryTicks` of silence, and ignores duplicate or unsolicited responses.
- More than `MaxRetries` retransmissions of one message ends the session with `ErrGiveUp` (wrapped with `%w`), so a dead link cannot make the client spin forever.
- An `ERR` during the handshake ends the session with `ErrRejected`.
- It validates the WELCOME against its own hello (version in range, features only ones it offered).
- When every request is answered it sends BYE once and is `Done` with no error.

Server behaviour: it answers a HELLO with WELCOME (again for a repeated HELLO), refuses REQ before a session with `ERR`, answers REQ with RESP, replies `ERR` to short REQ payloads and unknown types and **keeps going**, and stops after BYE. Bad bytes are the scanner's business.

## Limits of the model

The pipe never reorders, never delays beyond one tick, and its faults are random but bounded per run. It shows that your logic survives *this* fault model. It says nothing about congestion, reordering, security or real clocks. Selecting `RetryTicks` is a modelling choice, not a measurement. Justifying why requests may be retransmitted is a design argument that cannot be machine-graded here.
