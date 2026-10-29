**Challenge: many errors at once.**

The starter has the lesson's `RangeError`, `ParseAge` and `ErrDivideByZero`. Add two functions:

```go
func ParseAges(inputs []string) ([]int, error)
func RangeErrors(err error) []*RangeError
```

`ParseAges` parses every string with `ParseAge`. It returns the ages that were valid, **in input order**, and a single error that combines every failure (use `errors.Join`). The error is `nil` when everything parsed. Each failure is wrapped with its position, written `input N: ` (N counts from 0) followed by the original error, using `%w` so that `errors.Is` and `errors.As` still see through it. For `[]string{"30", "x", "200"}` the ages are `[30]` and the error text has two lines, one beginning `input 1: parse age:` and one `input 2: 200 is outside 0..150`.

`RangeErrors` walks any error tree and returns every `*RangeError` inside it, in the order they are found. A joined error exposes its parts through an `Unwrap() []error` method; ordinary wrappers have `Unwrap() error`. `errors.As` only finds the first match, so you must walk the tree yourself. A `nil` error gives an empty (or nil) result.

Do not wrap a nil error, and do not return a non-nil error when there was nothing wrong.
