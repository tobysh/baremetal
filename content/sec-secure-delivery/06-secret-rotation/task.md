Implement `NewKeyRing`, `Rotate`, `Revoke`, `Check` and `Current` (the types and error values are supplied). `NewKeyRing(grace)` takes the grace period in clock ticks; treat a negative value as 0. Beware overflow when computing `now + grace`.

`main` rotates a ring twice, checks both secrets during and after the grace period and revokes one. The hidden tests replay hand-worked incident timelines and thousands of random schedules of rotations, revocations and checks, comparing every `Check` with an independent model built from the event list.
