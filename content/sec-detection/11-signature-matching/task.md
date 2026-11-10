Stdin: signature lines `SIG <sid> <offset> <depth> <case|nocase> <hexpattern>` (at most 32), a blank line, then payload lines of hex (at most 200 lines, up to 512 bytes each). The pattern matches if it occurs fully inside `payload[offset .. offset+depth)` clipped to the payload (depth 0 means up to the end). For `nocase` fold ASCII A-Z/a-z on both pattern and payload.

For payload N (1-based) print `payload N: ` followed by the matching sids in signature order separated by single spaces, or `-` if none, or `invalid` if the hex line has an odd length or any non-hex character. Tests use several signature sets and payloads, including malformed lines.

Details: `offset` and `depth` are non-negative decimal numbers; patterns are valid hex with at most 256 bytes; hex digits in payloads may be upper or lower case; a payload that is valid but shorter than every pattern prints `-`; an invalid payload prints only `invalid` and no sids; a signature list is never empty in the tests.
