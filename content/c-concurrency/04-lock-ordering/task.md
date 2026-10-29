`Account` and `account_init` are in the starter. Implement `int transfer(Account *from, Account *to, long amount)`:

- Return `0` and change nothing if `from` and `to` are the same account, if `amount <= 0`, or if `from` has less than `amount`.
- Otherwise move `amount` from `from` to `to` and return `1`.
- Many threads call `transfer` on the same accounts at once, in any direction. It must never deadlock, and the total money must stay the same.
- Lock the two accounts in order of their `id` (the lower id first).
