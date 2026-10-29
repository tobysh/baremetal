Split the work: find the scheme, find the authority, then split off `@`, then the port. Test each condition in the order listed so that the first failing reason wins.
===
Lowercase the host into a small buffer (reject anything longer than, say, 100 characters as malformed). Strip one trailing dot after lowercasing. Reuse the wildcard matcher from the first lesson.
===
Internal check: with `a = ip >> 24` and `b = (ip >> 16) & 255`: `a == 0 || a == 10 || a == 127 || a >= 224 || (a == 100 && b >= 64 && b <= 127) || (a == 169 && b == 254) || (a == 172 && b >= 16 && b <= 31) || (a == 192 && b == 168)`.
