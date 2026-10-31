Write the rules as a series of `if ... { return Err... }` in the listed order. Early returns keep the precedence right.
===
Beware unsigned arithmetic: compare counters directly (`o.Counter <= st.HighWater`) and never subtract them.
===
For the clock: `o.IssuedAt > now+Skew` and `now >= o.ExpiresAt`. The first is exclusive of the boundary, the second inclusive.
===
Rule 5 must come before rule 6: after a normal install `Counter == HighWater`, and replaying it should say "already installed", not "downgrade".
