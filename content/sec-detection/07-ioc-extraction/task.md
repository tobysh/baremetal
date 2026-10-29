Read stdin (at most 200 lines of at most 400 characters). Print every distinct indicator exactly once as `ip <addr>`, `domain <name>` or `hash <hex>`, ordered by type (ip, then domain, then hash) and within a type by `strcmp` on the printed value. Trim trailing and leading dots from tokens and treat `:` `/` `?` `,` `(` `)` and spaces as separators. Print `no indicators` if there are none.

Tests use several reports, including invalid addresses, wrong-length hex and mixed-case values.
