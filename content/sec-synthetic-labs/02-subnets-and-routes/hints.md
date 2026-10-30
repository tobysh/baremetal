Use net/netip to parse and mask prefixes; use uint64 for the /0 address count.
===
Handle /31 and /32 before subtracting reserved addresses. For routing, retain the matching prefix with the most bits.
