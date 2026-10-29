`time_make` builds `Time t = {hours, minutes};` and returns it. `time_to_minutes` is `t.hours * 60 + t.minutes`. `time_from_minutes` is the reverse, using `/ 60` for the hours and `% 60` for the minutes.
===
Let the helpers do the work: `time_add` can convert both times to minutes, add them, and convert back with `time_from_minutes`. `time_before` compares the two minute totals.
===
```c
Time time_add(Time a, Time b) {
    return time_from_minutes(time_to_minutes(a) + time_to_minutes(b));
}

int time_before(Time a, Time b) {
    return time_to_minutes(a) < time_to_minutes(b);
}
```
