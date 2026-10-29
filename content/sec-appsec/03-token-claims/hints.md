Split the order into stages: parse, authenticate, then claims; an unauthenticated payload must never influence the verdict.
===
`expired` and `not-yet-valid` use the skew in opposite directions; test the exact boundaries.
