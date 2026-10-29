Implement the `Bank`.

- `Bank::new(balances: &[i64]) -> Bank`: one account per balance.
- `balance(&self, i: usize) -> Option<i64>`: the balance, or `None` if there is no such account.
- `total(&self) -> i64`: the sum of all balances.
- `deposit(&self, i: usize, amount: i64) -> Result<(), String>`: add `amount` (it may be negative); `Err("no such account")` for a bad index.
- `transfer(&self, from, to, amount) -> Result<(), String>` checks in this order and returns the first problem: `"no such account"` if either index is out of range, `"same account"` if `from == to`, `"invalid amount"` if `amount <= 0`, `"insufficient funds"` if `from` holds less than `amount`. Otherwise it moves the money atomically. **Lock the lower-numbered account first** whatever the direction.
- `parallel_deposits(bank: &Arc<Bank>, account: usize, amounts: Vec<i64>)`: spawn one thread per amount that deposits it into `account`, and join them all before returning.
