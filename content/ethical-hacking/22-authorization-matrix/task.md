Implement `audit_access(target, probes, n, verdicts)` and return the number of confirmed bypasses.

For each probe call `request(ctx, owner, object, operation)` once. Success means status 200–299 inclusive and `effect != 0`. If that control fails, write `INCONCLUSIVE` and make no replay. Otherwise call once with `other`, preserving object and operation. Write `BYPASS` for a successful replay, otherwise `ENFORCED`. Store one verdict per probe in input order. Do not make other calls or modify the probes. With `n == 0`, make no calls and return 0; the arrays may then be NULL. For positive n, pointers are valid, identities differ and n is at most 32. The callback returns values by value and isolates requests.

Hidden tests vary IDs, operations, statuses and effects, including misleading 2xx replies and failed controls.
