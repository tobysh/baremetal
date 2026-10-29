Keep an array of live nonces with their message times; rebuild it (compact) at step 2 for each message.
===
The window edges are inclusive: `msg-time == receive-time - W` is accepted.
