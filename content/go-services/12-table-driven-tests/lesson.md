---
title: Table-Driven Tests
summary: Write a table of test cases that is good enough to catch deliberately broken versions of the function.
minutes: 40
difficulty: medium
tags: testing, table-driven
id: go-services/table-driven-tests
mode: harness
---
The standard Go testing style is a **table**: a slice of cases, each with its inputs and expected output, and one loop that checks them all.

```go
func TestAbs(t *testing.T) {
	tests := []struct {
		name string
		in   int
		want int
	}{
		{"positive", 3, 3},
		{"negative", -3, 3},
		{"zero", 0, 0},
	}
	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			if got := Abs(tc.in); got != tc.want {
				t.Errorf("Abs(%d) = %d; want %d", tc.in, got, tc.want)
			}
		})
	}
}
```
Why this works so well:

- Adding a case is one line, so you add more.
- `t.Run(name, ...)` makes each case a **subtest**: it is reported by name, you can run one with `go test -run 'TestAbs/negative'`, and one failure does not hide the others.
- The failure message prints the input *and* both values, so you rarely need a debugger.

## Which cases?

A table is only as good as its cases. Look for **boundaries and special values**: zero, the exact edges of a range, negative numbers, and "two inputs the wrong way round". A test suite that only checks the happy path will happily pass on broken code.

You can measure that: **mutation testing** deliberately breaks the code (a `<` becomes `<=`, a branch is deleted) and checks that some test fails. A mutant that survives points at a missing case. This exercise does exactly that to your table.
