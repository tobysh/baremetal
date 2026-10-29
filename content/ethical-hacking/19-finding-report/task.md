**1. `int render_report(const ReportIn *in, char *out, size_t cap)`** writes exactly:

```text
FINDING: <title>\n
SEVERITY: <name>\n
ROOT CAUSE: <root_cause>\n
EVIDENCE:\n
- <evidence[0]>\n
- ...
REMEDIATION: <remediation>\n
```

`severity` 0 to 4 is `Info`, `Low`, `Medium`, `High`, `Critical`; an out-of-range value is written as `Info`. Every `\n` and `\r` inside a value is replaced by a space. Return the length written, or `-1` if the whole text plus its terminating NUL does not fit in `cap` (then write `out[0] = '\0'` if `cap > 0` and nothing else). Never write beyond `cap`.

**2. `int lint_report(const char *text)`** returns a bitmask (0 means fine). A label line starts a line with `FINDING:`, `SEVERITY:`, `ROOT CAUSE:`, `EVIDENCE:` or `REMEDIATION:`; the value is the rest of that line with surrounding blanks removed. Evidence bullets are the lines starting with `- ` after `EVIDENCE:` and before the next label.

- `1` FINDING missing or empty
- `2` SEVERITY missing or not exactly one of `Critical`, `High`, `Medium`, `Low`, `Info`
- `4` ROOT CAUSE missing or empty
- `8` no evidence bullets
- `16` REMEDIATION missing or shorter than 15 characters
- `32` an evidence bullet contains `password=`, `token=` or `secret=` followed by anything other than `[redacted]`
- `64` the labels that are present are not in the order above

Many generated reports are checked.
