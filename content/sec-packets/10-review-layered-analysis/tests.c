static uint32_t st = 1010;
static uint32_t rnd(void) { st = st * 1664525u + 1013904223u; return st >> 8; }

static void w16(uint8_t *p, unsigned v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }

/* Builds a frame. The IP packet is `ihl*4 + tlen` bytes (its total length); `pad` extra bytes follow it
 * in the capture. A TCP transport has data offset 5; a UDP transport declares length `udplen`.
 * Option bytes (ihl > 5) are zero. Returns the number of bytes written. */
static size_t build(uint8_t *b, int vlan, unsigned etype, unsigned ihl, unsigned proto, unsigned ff, unsigned sport, unsigned dport,
                    unsigned tlen, unsigned udplen, unsigned pad)
{
    size_t n = 0;
    for (int i = 0; i < 12; i++) b[n++] = (uint8_t)rnd();
    if (vlan) { b[n++] = 0x81; b[n++] = 0; b[n++] = 0; b[n++] = 7; }
    b[n++] = (uint8_t)(etype >> 8); b[n++] = (uint8_t)etype;
    size_t ip = n;
    memset(b + ip, 0, ihl * 4 + tlen + pad);
    b[ip] = (uint8_t)(0x40 | ihl);
    w16(b + ip + 2, ihl * 4 + tlen);
    w16(b + ip + 6, ff);
    b[ip + 8] = 64;
    b[ip + 9] = (uint8_t)proto;
    b[ip + 12] = 192; b[ip + 14] = 2; b[ip + 15] = 10;
    b[ip + 16] = 198; b[ip + 17] = 51; b[ip + 18] = 100; b[ip + 19] = 20;
    uint8_t *t = b + ip + ihl * 4;
    if (tlen >= 4) { w16(t, sport); w16(t + 2, dport); }
    if (proto == 6 && tlen >= 13) t[12] = 0x50;
    if (proto == 17 && tlen >= 6) w16(t + 4, udplen);
    for (unsigned i = 0; i < pad; i++) b[ip + ihl * 4 + tlen + i] = (uint8_t)(1 + rnd() % 255);   /* padding bytes */
    return ip + ihl * 4 + tlen + pad;
}

#define IS(label, n) (strcmp(classify(b, (n)), (label)) == 0)

int main(void)
{
    uint8_t b[256];
    size_t n;

    n = build(b, 0, 0x0806, 5, 0, 0, 0, 0, 20, 0, 0);
    CHECK("ARP stops at Ethernet", IS("arp", 14) && IS("arp", n), "EtherType 0x0806 returns arp without parsing a body, got \"%s\"", classify(b, 14));
    n = build(b, 1, 0x0806, 5, 0, 0, 0, 0, 20, 0, 0);
    CHECK("ARP inside a VLAN tag", IS("arp", n), "the tag is unwrapped before the type decision, got \"%s\"", classify(b, n));

    n = build(b, 0, 0x86dd, 5, 0, 0, 0, 0, 40, 0, 0);
    CHECK("other EtherTypes", IS("other", n), "IPv6 is not decoded by this lesson, got \"%s\"", classify(b, n));

    {
        unsigned ports[][2] = {{40000, 80}, {1, 65535}, {60001, 22}};
        int bad = -1;
        for (int i = 0; i < 3; i++) {
            n = build(b, i == 2, 0x0800, 5, 6, i == 1 ? 0x4000 : 0, ports[i][0], ports[i][1], 20, 0, (unsigned)(i * 6));
            if (!IS("ipv4/tcp", n)) { bad = i; break; }
        }
        CHECK("complete TCP, with DF, a VLAN tag and padding", bad < 0, "case %d returned \"%s\" (want ipv4/tcp; Don't Fragment alone is not a fragment)", bad, bad < 0 ? "" : classify(b, n));
    }

    n = build(b, 0, 0x0800, 5, 6, 0, 1, 2, 19, 0, 30);
    CHECK("TCP cut short by the IP total length", IS("invalid", n), "the IP total ends 19 bytes into the segment even though padding follows; got \"%s\"", classify(b, n));

    n = build(b, 0, 0x0800, 5, 17, 0, 40000, 8080, 8, 8, 0);
    CHECK("UDP without a DNS port", IS("ipv4/udp", n), "ports 40000 and 8080 give ipv4/udp, got \"%s\"", classify(b, n));
    n = build(b, 0, 0x0800, 5, 17, 0, 53, 40000, 12, 12, 10);
    int src53 = IS("ipv4/udp/dns-port", n);
    n = build(b, 0, 0x0800, 5, 17, 0, 40000, 53, 8, 8, 0);
    CHECK("source or destination port 53", src53 && IS("ipv4/udp/dns-port", n), "port 53 on either side gives ipv4/udp/dns-port, got \"%s\"", classify(b, n));

    n = build(b, 0, 0x0800, 5, 17, 0, 1000, 2000, 12, 20, 16);
    CHECK("UDP length cannot reach into padding", IS("invalid", n), "the datagram claims 20 bytes but the IP total allows 12; 16 padding bytes follow in the capture; got \"%s\"", classify(b, n));

    n = build(b, 0, 0x0800, 8, 17, 0, 40000, 53, 8, 8, 0);
    CHECK("IP options shift the transport header", IS("ipv4/udp/dns-port", n), "IHL 8 means the UDP header starts at IP byte 32, got \"%s\"", classify(b, n));

    {
        n = build(b, 0, 0x0800, 5, 6, 0x2000, 80, 80, 3, 0, 0);
        int mf = IS("ipv4/fragment", n);
        n = build(b, 0, 0x0800, 5, 17, 0x00b9, 0, 0, 2, 0, 0);
        int offset = IS("ipv4/fragment", n);
        CHECK("fragments are labelled before transport parsing", mf && offset, "More Fragments with a 3-byte body and a nonzero offset with 2 bytes must give ipv4/fragment, got \"%s\"", classify(b, n));
    }

    {
        n = build(b, 0, 0x0800, 5, 1, 0, 8, 0, 8, 0, 0);
        int icmp = IS("ipv4/other", n);
        n = build(b, 0, 0x0800, 5, 47, 0, 0, 0, 4, 0, 0);
        CHECK("other IP protocols", icmp && IS("ipv4/other", n), "protocols 1 and 47 give ipv4/other, got \"%s\"", classify(b, n));
    }

    {
        n = build(b, 0, 0x0800, 5, 6, 0, 1, 2, 20, 0, 0);
        int ok = IS("ipv4/tcp", n);
        b[14] = 0x65;
        int version = IS("invalid", n);
        b[14] = 0x45;
        int ip_cut = IS("invalid", 14 + 19);
        int eth_cut = IS("invalid", 13) && IS("invalid", 0);
        CHECK("malformed headers are invalid", ok && version && ip_cut && eth_cut, "IPv6 version nibble: %d; IP header truncated to 19 bytes: %d; Ethernet truncated: %d (want 1 each)", version, ip_cut, eth_cut);
        CHECK("NULL frame", strcmp(classify(NULL, 60), "invalid") == 0, "NULL must return invalid");
    }
    return 0;
}
