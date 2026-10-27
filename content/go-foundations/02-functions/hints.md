In Go, `a / b` and `a % b` give the quotient and remainder of integer division. Check `b == 0` first and return early.
===
For `MinMax`: if `len(nums) == 0` return. Otherwise start with `min, max = nums[0], nums[0]` and loop over `nums`, updating them. Named results can be assigned directly.
===
`return a / b, a % b` returns both values. In `MinMax` remember a negative number can be the largest if all values are negative, so don't start `max` at 0.
