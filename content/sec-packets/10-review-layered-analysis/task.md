The starter already contains the four bounded parsers from earlier lessons (`parse_ethernet`, `parse_ipv4`, `parse_tcp`, `parse_udp`) with their structs. Keep them as they are and implement `const char *classify(const uint8_t *frame, size_t n)`, which returns one of the exact strings below (string literals; the tests compare with `strcmp`).

**Walk the layers in this order:**

1. `parse_ethernet(frame, n, ...)` fails: `"invalid"`. A frame of `NULL` is invalid too.
2. EtherType `0x0806`: `"arp"` (an EtherType observation only; the ARP body is not parsed). Any other type that is not `0x0800` (IPv6, unknown): `"other"`. These two decisions use the type after one VLAN tag is unwrapped.
3. EtherType `0x0800`: `parse_ipv4` on the bytes from `Ethernet.payload` to the end of the frame. Failure: `"invalid"`.
4. If the IPv4 packet is fragmented (`fragmented != 0`): `"ipv4/fragment"`. Do this **before** decoding a transport header.
5. The transport span starts at `payload + header` and is exactly `total - header` bytes long. Bytes past the IP total length (padding) must not be given to the transport parser.
6. Protocol 6: `"ipv4/tcp"` if `parse_tcp` succeeds on that span, else `"invalid"`.
7. Protocol 17: if `parse_udp` fails, `"invalid"`. Otherwise `"ipv4/udp/dns-port"` when the source or destination port is 53, else `"ipv4/udp"`.
8. Any other IP protocol: `"ipv4/other"`.

The tests include VLAN-tagged frames, IP options (IHL above 5), fragments whose transport bytes are garbage, trailing padding, packets truncated at the IP level and inside TCP or UDP, and UDP lengths that reach into padding. Keep the supplied function names and types.
