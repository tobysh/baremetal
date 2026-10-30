`Quote` walks `rules` in order and stops at the first rule whose `match` returns true. Start every question by listing which rules could match.
===
After the rule's base price: express adds half (`c / 2`, rounded down), then a member discount removes a tenth (`c / 10`, rounded down), then the price is clamped to `minCents`/`maxCents`. The `member-local` rule returns early and skips all of that.
===
`abroad` comes before `heavy`, so a heavy parcel to zone 3 is an `abroad` parcel. Remember the maximum clamp.
===
For A5: which rule matches zone 0? Compute its base, then the minimum clamp.
===
For A6 work out the `standard` price as a function of grams: `300*zone + 40*ceil(grams/250)`. The letter rule only applies below 100 grams, and `letter` is cheap anyway.
