Work top-down and return early. After `parse_ethernet`, branch on `e.type` for ARP and "other" before you parse any IP bytes. The IPv4 packet starts at `frame + e.payload` and has `n - e.payload` bytes available.
===
Check `ip.fragmented` immediately after `parse_ipv4`. A later fragment may be only a few bytes with no TCP or UDP header; decoding it would report "invalid" instead of the right label.
===
Compute the transport pointer as `frame + e.payload + ip.header` and its length as `ip.total - ip.header`. A common bug is passing `n - e.payload - ip.header`, which includes Ethernet padding.
===
For UDP, only the port numbers matter for the label: `u.src == 53 || u.dst == 53` gives `ipv4/udp/dns-port`. For TCP, a successful parse is enough. Any protocol other than 6 and 17 returns `ipv4/other` once the IPv4 header has parsed.
