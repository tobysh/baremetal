---
title: UDP Datagrams
summary: UDP sends self-contained messages with no connection or guarantees. Bind a datagram socket, reply to the sender and give up politely when no answer comes.
minutes: 25
difficulty: medium
tags: udp, sockets, datagrams
id: c-networking/udp
mode: harness
---
TCP gives you a reliable **stream**. UDP gives you **datagrams**: independent messages, each one sent and received whole.

| | TCP | UDP |
|---|---|---|
| Connection | yes (`connect`, `accept`) | none |
| Boundaries | none, bytes only | each `sendto` is one message |
| Reliability | retransmits, in order | may be lost, duplicated, reordered |
| Typical use | web, mail, SSH | DNS, games, video, time sync |

## The calls

```c
int s = socket(AF_INET, SOCK_DGRAM, 0);
bind(s, (struct sockaddr *)&addr, sizeof addr);          // server only

sendto(s, buf, n, 0, (struct sockaddr *)&dest, sizeof dest);

struct sockaddr_in from;
socklen_t fl = sizeof from;
ssize_t got = recvfrom(s, buf, cap, 0, (struct sockaddr *)&from, &fl);
```

`recvfrom` also tells you **who sent it** (`from`), and that is how a UDP server replies: `sendto(s, reply, n, 0, &from, fl)` straight back to that address. There is no `accept`, because there is no connection to accept.

Three details that matter:

- **One call, one datagram.** If the datagram is larger than your buffer, the extra bytes are *thrown away*, not saved for the next call.
- **A zero-length datagram is legal**, so `recvfrom` returning 0 does *not* mean end of file the way it does on TCP.
- **No answer is a normal outcome.** A client that waits forever for a lost reply hangs forever. Real UDP clients always use a timeout and often retry. `setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv)` makes `recvfrom` fail with `EAGAIN` or `EWOULDBLOCK` after a delay.

On loopback datagrams are rarely lost, but the code you write is the same as for the real Internet, and the tests check the timeout path.
