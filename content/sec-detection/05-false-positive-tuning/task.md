Stdin: lines `ALLOW <rule-glob> <src-glob> <user-glob>` (globs have no spaces), a blank line, then alerts `<rule> <src> <user>`. At most 50 allow lines and 500 alerts. An alert is suppressed by the first ALLOW line whose three globs all match.

Print `kept=K suppressed=S`, then for each allow line in order `allow I hits=H` (with ` stale` appended when H is 0), then each kept alert on its own line as `kept: <rule> <src> <user>` in input order. Tests use several allowlists, including patterns where only `?` or only `*` distinguishes the result.
