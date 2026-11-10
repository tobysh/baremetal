Stdin: rules `<allow|deny> <tcp|udp|any> <src> <dst> <ports>` where src and dst are `any`, an IPv4 address (a single host) or `a.b.c.d/len`, and ports is `any`, a number or `lo-hi` (the destination port). A blank line ends the rules. Then packets `<proto> <src-ip> <dst-ip> <dst-port>` where proto is `tcp`, `udp` or `icmp` (icmp matches only `any` rules). At most 64 rules and 500 packets.

Print `packet N: allow (rule K)` or `packet N: deny (rule K)` for the first matching rule, or `packet N: deny (default)` if none match. Tests use many rule sets, including `/0` and `/32` networks and non-normalised networks like `10.1.2.3/8`.

Details: ports `lo-hi` are inclusive and rules have `lo <= hi`; a bare address in a rule is a `/32`; `any` as a network is the same as `0.0.0.0/0`; all addresses and prefix lengths are valid; a packet with proto `icmp` carries a port of `0` and only `any` protocol rules can match it; rule numbers in the output are 1-based.
