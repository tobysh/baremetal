Keep all events; for each new event scan back over earlier events with the same key while their time is still inside the window.
===
Store the time until which each key is suppressed; compare with `t >= suppress_until`.
