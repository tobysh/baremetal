Write a private `refill(now int64)` method and call it from both `Allow` and `Tokens`. Start with rule 1 so negative elapsed time never reaches the division.
===
Rule 2: when `tokens == capacity` set `last = now` and return. That stops idle time from being banked as credit.
===
Rule 3: `n := (now - last) / interval`. If `n >= int64(capacity-tokens)` then `tokens = capacity; last = now`. Otherwise `tokens += int(n); last += n * interval`.
===
Do not write `tokens + int(n)` before comparing; a huge `n` overflows `int`.
