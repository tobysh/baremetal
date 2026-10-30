`SumTo`: `total := 0`, then `for i := 1; i <= n; i++ { total += i }`. If `n` is not positive the loop body never runs, so the answer is already 0.
===
`Collatz`: `steps := 0`, then `for n != 1 { ... steps++ }`. Inside, `if n%2 == 0 { n /= 2 } else { n = 3*n + 1 }`.
===
`DayKind`: one `case` can list several values: `case "mon", "tue", "wed", "thu", "fri":`. Finish with `default:`.
===
`IndexOf` and `SumEvensUntil`: `for i, v := range xs`. In `IndexOf`, return `i` as soon as `v == target`, and `-1` after the loop. In `SumEvensUntil`, test `v > limit` first and `break`, then `if v%2 != 0 { continue }`, then add. Remember `-3 % 2` is `-1`, so test `!= 0` rather than `== 1`.
