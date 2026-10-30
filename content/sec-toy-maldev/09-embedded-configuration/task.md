Implement three functions.

```go
type Config struct {
	Label    string   // required, 1-32 bytes
	Interval uint16   // default 10 when absent
	MaxOps   uint16   // default 64 when absent
	Allow    []string // default empty (non-nil) when absent
	Token    string   // empty means unset
}

func Decode(b []byte) (Config, error)
func Encode(c Config) ([]byte, error)
func Describe(c Config) string
```

`Decode`

- Layout: `BCFG`, version byte (1 or 2), uint16 LE body length, body. The body length must equal the bytes that follow exactly (no truncation, no trailing bytes). Body at most 1024 bytes.
- A field is tag, length byte, value. Tags: 1 label (1-32 bytes, required), 2 interval (uint16 LE, nonzero), 3 MaxOps (uint16 LE, 1-4096), 4 Allow list (v2: count 0-8, then length-prefixed keys of 1-32 bytes, unique, nothing left over), 5 token (v2: 1-64 bytes).
- Tag 2 and 3 values must be exactly two bytes. Duplicate known tags are an error. A tag 4 or 5 inside a version-1 record is an error. Any other tag below 0x80 is critical and an error. Tags 0x80 and above are skipped, whatever their value, but their length must still fit in the body.
- Apply defaults: Interval 10, MaxOps 64, Allow `[]string{}`. Return `Config{}` and an error for any violation. Returned strings and slices must not alias `b`.

`Encode`

- Validate with the same value rules (label 1-32 bytes, Interval nonzero, MaxOps 1-4096, Allow at most 8 unique keys of 1-32 bytes, Token at most 64 bytes). Return `nil, error` otherwise.
- Version 1 when `len(Allow) == 0 && Token == ""`, else version 2. Write tags in ascending order and omit tag 2 when Interval is 10, tag 3 when MaxOps is 64, tag 4 when Allow is empty and tag 5 when Token is empty. An Allow list that is present is written even if it equals nothing else.
- `Decode(Encode(c))` must equal `c` (with an empty Allow normalized to `[]string{}`).

`Describe`

- Return one line that contains the label, interval, MaxOps and the allowed keys joined by commas, for example `label=lab-1 interval=10 maxops=64 allow=a,b token=unset`, or `token=set(len=12)` when a token is present. It must not contain the token, its hex, its base64 or its bytes XORed with a single byte.

Example: `BCFG 01 0a 00 ...` with only a label `lab` is `42 43 46 47 01 05 00 01 03 6c 61 62`.
