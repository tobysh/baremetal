`Time` is defined in the starter, with `minutes` in the range 0 to 59 in every value you return. Hours never wrap, so `25:10` is possible.

- `Time time_make(int hours, int minutes)` returns a time with those fields. The minutes passed in are always already 0 to 59.
- `Time time_from_minutes(int total)` returns the time that many minutes long. `total` is not negative. `130` becomes 2 hours 10 minutes.
- `int time_to_minutes(Time t)` returns the total number of minutes.
- `Time time_add(Time a, Time b)` returns the sum, normalised so that `minutes` stays below 60.
- `int time_before(Time a, Time b)` returns `1` if `a` is strictly earlier than `b`, otherwise `0`.
