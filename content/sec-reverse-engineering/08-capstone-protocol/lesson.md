---
title: "Capstone: reverse a toy protocol"
summary: Work out a made-up request and reply protocol from captured frames, then re-implement the responder, including its error handling and check order.
minutes: 75
difficulty: hard
tags: security, reverse-engineering, protocols, capstone
id: sec-reverse-engineering/capstone-protocol
mode: stdout
---
This capstone brings the track together. `bmq` is a made-up request and reply protocol invented for this course. You have never seen its specification, only a capture of frames sent to a responder and the frames it sent back. Your task is to infer the rules and re-implement the responder. There is no network and no real device: frames are hex text on standard input, and your program prints the replies.

The capture (request, then the reply the responder gave):

```
b7 01 01 04 70 69 6e 67 f9           ->  b7 81 01 04 70 69 6e 67 79
b7 02 02 06 61 62 63 64 65 66 ec     ->  b7 82 02 06 66 65 64 63 62 61 6c
b7 03 03 03 01 02 fa 17              ->  b7 83 03 02 00 fd 92
b7 04 04 0e 48 65 6c 6c 6f 2c 20 57 6f 72 6c 64 20 39 f6
                                     ->  b7 84 04 0e 48 45 4c 4c 4f 2c 20 57 4f 52 4c 44 20 39 76
b7 01 05 00 e9                       ->  b7 81 05 00 69
b7 09 06 02 68 69 e1                 ->  b7 ff 06 01 01 14
b7 01 07 02 68 69 f8                 ->  b7 ff 07 01 02 16
b7 09 08 02 68 69 ff                 ->  b7 ff 08 01 02 19
b7 01 09 05 68 69 f1                 ->  b7 ff 09 01 03 19
b8 01 0a 02 68 69 ea                 ->  (no reply; the viewer shows "drop")
b7 01 0b                             ->  (no reply; "drop")
```

Approach it like the earlier lessons. Start with the anchors: every frame starts with `b7`, the second byte differs between operations, and the third byte counts up through the capture, so it is a sequence number that the reply copies. The fourth byte is the payload length. Operations 1 to 4 each do something to the payload; compare payload and reply to see what. The reply's operation code is related to the request's by a simple rule. The last byte is a check byte: find how it is computed from the others by trying the obvious candidates (a sum, an xor, an xor with a constant) on a frame whose answer you know. The reply carries a check byte too.

Then study the failures, which are the part most analysts under-sample. There are three kinds of error reply and two silent drops. Work out what each one is, and, using the frames where two faults occur together (the eighth and ninth requests), which check comes first. This is exactly the kind of thing a hidden test varies, so do not guess: derive it from the evidence.

Be honest about the limits of what you have inferred. Eleven frames cannot rule out behaviour they never exercised (for example, what happens with payloads larger than any shown), and the model you build is only as good as the capture. In a real report you would list these as assumptions, as in the [claims lesson](/learn/sec-reverse-engineering/claims-evidence).
