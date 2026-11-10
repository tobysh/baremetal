Implement `Aggregate(rows []Row) []Group`, the executor for `GROUP BY Key` computing `COUNT(*)`, `COUNT(Value)` and `SUM(Value)`.

- **Grouping:** rows with equal valid keys share a group. All rows whose key is NULL (`Key.Valid == false`) share exactly one group, whatever garbage is in their `N` field. The key `0` is not NULL and is a separate group.
- **Output key:** `Group.Key` is the group's key; for the NULL group it must be `Value{}` (`N == 0`, `Valid == false`).
- **Order:** groups appear in order of the first row that belongs to them. No sorting.
- `Rows` is the number of rows in the group. `Count` is how many of them have `Value.Valid`. `Sum` is the sum of the valid values, and `Sum.Valid` is true **exactly when** `Count > 0`. When `Sum.Valid` is false, `Sum.N` must be 0. A sum that happens to equal 0 is still valid.
- Empty (or `nil`) input returns no groups.
- Do not modify `rows`. The tests also try many random tables, so a result tuned to the example will fail.
