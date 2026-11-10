For each packet, walk the rule list from the top and stop at the first rule where protocol, source, destination and port all match. Remember the index of that rule, because the output prints it.
===
Convert each address to a `uint32_t` (first octet in the top byte) and store each rule's network and mask. A packet address is inside a network when `(addr & mask) == (net & mask)`. A bare address means `/32`; `any` is `0.0.0.0/0`.
===
If `/0` rules misbehave, you shift by 32; build the mask as `len == 0 ? 0 : 0xFFFFFFFFu << (32 - len)`. If `10.1.2.3/8` never matches `10.200.0.1`, you did not mask the rule's own network. If icmp packets are allowed by a `tcp` rule, your protocol test treats icmp as a wildcard; only a rule with protocol `any` may match it.
===
The same test on a different example, "is this host in a /20?":
```c
uint32_t m = 0xFFFFFFFFu << (32 - 20);
int inside = (host & m) == (net & m);
```
