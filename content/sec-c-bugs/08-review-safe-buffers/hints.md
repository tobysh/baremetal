This is a review: make every validation happen before the first write.
===
After rejecting a negative length, `n < cap` reserves the terminator without risky addition.
