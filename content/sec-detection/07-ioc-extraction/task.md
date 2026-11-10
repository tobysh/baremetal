Read stdin (at most 200 lines of at most 400 characters). Print every distinct indicator exactly once as `ip <addr>`, `domain <name>` or `hash <hex>`, ordered by type (ip, then domain, then hash) and within a type by `strcmp` on the printed value. Trim trailing and leading dots from tokens and treat `:` `/` `?` `,` `(` `)` and spaces as separators. Print `no indicators` if there are none.

Tests use several reports, including invalid addresses, wrong-length hex and mixed-case values.

Details: leading zeros make an address invalid (`01.2.3.4`), but a lone `0` part is fine; a token such as `10.2.3.4.5` is dropped entirely; only domains ending in `.test` count and the token needs at least two labels (`.test` alone and `test` are not domains); a hex run of any other length is not a hash; hashes and domains are printed lowercase; IP addresses are printed as found, and sorting is plain `strcmp` on the text.
