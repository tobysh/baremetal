# Synthetic packet evidence

Run `python3 scripts/content-gen/sec-packets.py` to reproduce these byte-for-byte.
All timestamps, MACs and payloads are invented. IPs are from documentation ranges
192.0.2.0/24, 198.51.100.0/24 and 203.0.113.0/24. Names use example.test.
There is no live capture, external traffic, personal data or embedded secret.

Open sample.pcap in Wireshark, or run `tshark -r content/sec-packets/fixtures/sample.pcap`.
The big-endian variant contains the same frames and tests capture byte-order handling.
TCP checksums in this teaching fixture are zero (the lesson does not validate them);
UDP zero checksums are permitted for IPv4. IPv4 headers have valid checksums.

Useful display filters (not capture filters):

| Lesson | Wireshark display filter or view |
| --- | --- |
| pcap | frame; inspect Frame captured length / length |
| Ethernet | eth.type == 0x0800; vlan for tagged examples |
| ARP | arp; arp.opcode == 1 |
| IPv4 | ip; ip.flags.mf == 1 || ip.frag_offset > 0 |
| TCP | tcp; tcp.flags.syn == 1 && tcp.flags.ack == 0 |
| UDP | udp; udp.port == 53 |
| DNS | dns; dns.qry.name == "notes.example.test" |
| HTTP stream | tcp.port == 80; Follow > TCP Stream |
| Anomaly | tcp.flags.syn == 1 && tcp.flags.ack == 0; Statistics > Conversations |
| Review | arp || ip |

The anomaly lesson uses synthetic flow summaries in its harness, not additional
real traffic. Malformed parser cases remain embedded in tests rather than mixed
into the valid demonstration capture. Display filters select observations;
they do not establish that a source is malicious.
