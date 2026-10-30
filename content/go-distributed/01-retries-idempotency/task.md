Implement the four pieces in `starter.go`.

- `Server.Handle(r Request) Response` applies `r.Amount` to the balance at most once per `(Client, Seq)`. The first request of a client has `Seq` 1. When `r.Seq` is one more than the last applied sequence for that client, apply it and remember the reply. When `r.Seq` equals the last applied one, return the remembered reply unchanged. For any other `Seq` do not change the balance; return a `Response` with the right `Client` and `Seq` and the current balance. `Balance()` returns the current balance. Different clients are independent.
- `NewClient(id, amounts)` creates a client that must send `len(amounts)` requests, request `n` carrying `amounts[n-1]`. `Pending()` returns the request to send now (the same one on every call until acknowledged) and `false` once all are acknowledged. `OnResponse` advances only for a reply with this client's id and the sequence number in flight, and ignores everything else. `Done()` reports whether everything is acknowledged.

The hidden tests run many seeded schedules in which messages are dropped, duplicated and reordered, and they use negative and zero amounts. Do not assume the network is kind.
