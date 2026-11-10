Handle each line in three stages: rewrite `[.]` and `(.)` as `.`, split the result into tokens of letters, digits, `.` and `-`, then classify each token. Keep the three stages separate in your code.
===
Write one strict validator per type: `is_ip` (exactly four parts, 1 to 3 digits, no leading zero unless the part is `0`, value at most 255), `is_domain` (ends in `.test`, labels non-empty, no hyphen at either end of a label) and a hash check (length 32 or 64, all hex). Lowercase a token before testing it, and trim dots from both ends first.
===
If `10.2.3.4.5` produces an address, your parser stops after four parts instead of requiring the token to end there. If the same indicator is listed twice, deduplicate after lowercasing, not before. If `.168.1.1` appears, you did not trim the leading dot.
===
A strict part check on a different field, a two-digit hour:
```c
int ok = isdigit((unsigned char)s[0]) && isdigit((unsigned char)s[1])
      && s[2] == '\0' && (s[0] - '0') * 10 + (s[1] - '0') < 24;
```
