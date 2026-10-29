Implement `const char *classify(const uint8_t *frame,size_t n)` using supplied helpers. Return invalid if Ethernet, IPv4 or a selected TCP/UDP header fails. EtherType 0x0806 returns arp (EtherType observation only); other non-IPv4 types return other. IPv4 fragments return ipv4/fragment. Complete TCP returns ipv4/tcp; complete UDP returns ipv4/udp/dns-port if either port=53, else ipv4/udp. Other IP protocols return ipv4/other. Respect IPv4 total length and IHL.

Tests use several inputs, including boundary values. Keep the supplied function names and types.
