Implement:

```go
type Job struct {
    ID       string // 1..32 bytes, unique
    Start    uint64 // tick of the first attempt, at most 1<<32
    Fails    int    // the first Fails attempts fail, 0..16
    MaxTries int    // total attempts allowed, 1..8
    Backoff  uint64 // base delay, at most 1<<20; must be > 0 when MaxTries > 1
    CancelAt uint64 // 0 means never; otherwise at most 1<<32
}
type Event struct {
    At   uint64
    ID   string
    Kind string
}

func Schedule(jobs []Job, horizon uint64) ([]Event, error)
```

Simulate on a virtual clock. 1 to 32 jobs are allowed and horizon is at most 1<<32. Any invalid input returns `nil, error`, and the input slice must never be modified.

Each job has one pending thing at a time: an attempt due at `at` (initially `Start`), or its cancellation at `CancelAt`. Cancellation wins when `CancelAt != 0 && CancelAt <= at`, and then the event is `{CancelAt, ID, "cancelled"}` at the cancellation tick, even if the retry would have been later.

Attempt number `n` (starting at 1) at tick `t` emits exactly one event at `t`:

- `done` if `n > Fails`, and the job ends;
- `gave-up` if the attempt fails and `n == MaxTries`, and the job ends;
- `retry` otherwise; the next attempt is due at `t + Backoff<<(n-1)`.

Always process the pending thing with the smallest tick; break ties by smaller ID. Only things due at tick `<= horizon` happen. After that, each job that is still live gets `{horizon, ID, "horizon"}`, in ID order. The result is therefore sorted by tick, with ID order within a tick. Jump between events; do not loop once per tick.

Other job lists with different orders, starts and limits are tried.
