Stdin: at most 2000 lines `<timestamp> <client> <qname> <qtype>`. Lowercase the name, take the last two dot-separated labels as the domain and the rest as the subdomain (a query for the bare domain counts as a query but has no subdomain). For each domain with `U >= 8` and `S >= 20 * U`, print `<domain> unique=<U> avg=<S/U integer division> queries=<Q> txt=<number of TXT or NULL queries>`, ordered by U descending and then domain ascending. Print `no tunnelling suspected` if none qualify.

Tests include busy CDN-style domains and long-but-few names that must not be flagged.

Details: a name with a single label or two labels is its own domain with no subdomain; query types are exactly `A`, `AAAA`, `MX`, `TXT`, `NULL` or similar uppercase words and only `TXT` and `NULL` count toward `txt=`; the same subdomain asked by different clients is still one distinct subdomain; domains with `U` below 8 are never printed whatever their query count.
