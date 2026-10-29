Stdin: first line `ttl` (seconds), then at most 1000 events in time order: `<time> issue <user> <token>` or `<time> use <token> <new-password>` (the password has no spaces; the token and user have at most 31 characters). Print `N: <result>`:

`issue`: `ok` (this token becomes the user's only current token; any previous token of that user becomes superseded). A token string issued before is `ignored-duplicate` and changes nothing.
`use`: the first that applies: `invalid` (never issued), `superseded`, `already-used`, `expired` (`time - issued >= ttl`), `weak-password` (fewer than 8 characters; the token stays usable), otherwise `ok user=<user>` and the token is used up.
