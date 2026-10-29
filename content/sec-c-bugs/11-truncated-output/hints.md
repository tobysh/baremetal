`snprintf` returns the length the whole text would need. The label fits only when that value is less than `cap`.
===
Handle `cap==0` before anything touches `out`, and clear `out[0]` on every other failure.
