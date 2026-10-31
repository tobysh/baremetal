Server first. `frames, _ := s.sc.Feed(p)`, then switch on `f.Type`. Reply frames come from `mustEncode`. A RESP payload is the 4 sequence bytes followed by the body reversed.
===
Client state: handshake done or not; per request `sentAt[i]` (-1 until first sent), `tries[i]` and `got[i]`. Put the sending logic in one `pump(now)` that both `Tick` and `OnData` call: retransmit overdue requests, then send new ones while the number of sent-but-unanswered is below `Window`.
===
Guard every decode: RESP shorter than 4 bytes, a sequence outside `1..len(reqs)`, a request never sent, or already answered must all be ignored. When every `got[i]` is true, emit BYE once and set done.
===
Failure handling: count retransmissions per message (HELLO included) and when one exceeds `MaxRetries` set `err = fmt.Errorf("%w: ...", ErrGiveUp)` and `done = true`.
