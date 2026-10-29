`frame` and `unframe` (CRC framing: `crc u32`, `len u32`, `payload`) are given. Implement the database and its transactions using the batch payload format in the lesson.

- `NewDB() *DB`, `(*DB) Get(key string) (string, bool)` and `(*DB) Log() []byte` (a copy of the log).
- `(*DB) Begin() *Tx`.
- `(*Tx) Put(key, value string)` and `(*Tx) Delete(key string)` record pending operations only.
- `(*Tx) Get(key string) (string, bool)` sees the transaction's own pending writes first, then the database.
- `(*Tx) Commit() error` appends **one** frame for the whole batch to the DB's log, then applies the operations in order. A transaction with no operations writes nothing. Calling `Commit` or `Rollback` on a finished transaction returns `ErrTxDone` (`Commit`) or does nothing (`Rollback`).
- `(*Tx) Rollback()` discards the pending operations. After it, `Commit` returns `ErrTxDone`.
- `Recover(log []byte) *DB` rebuilds a database from a log. It applies complete, valid frames in order and stops at the first bad one; the returned DB's log is exactly the valid prefix (a copy), and it can be used for further commits.

Whatever prefix of the log survives a crash, the recovered database must equal the state after some whole number of committed transactions.
