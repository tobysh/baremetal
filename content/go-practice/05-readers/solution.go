package main

import (
	"bufio"
	"fmt"
	"io"
	"strings"
)

func ParseKV(r io.Reader) (map[string]string, error) {
	m := map[string]string{}
	sc := bufio.NewScanner(r)
	n := 0
	for sc.Scan() {
		n++
		line := strings.TrimSpace(sc.Text())
		if line == "" || strings.HasPrefix(line, "#") {
			continue
		}
		k, v, ok := strings.Cut(line, "=")
		if !ok {
			return nil, fmt.Errorf("line %d: missing '='", n)
		}
		k = strings.TrimSpace(k)
		if k == "" {
			return nil, fmt.Errorf("line %d: empty key", n)
		}
		m[k] = strings.TrimSpace(v)
	}
	if err := sc.Err(); err != nil {
		return nil, err
	}
	return m, nil
}

func Stats(r io.Reader) (lines, words, bytes int, err error) {
	data, err := io.ReadAll(r)
	if err != nil {
		return 0, 0, 0, err
	}
	s := string(data)
	lines = strings.Count(s, "\n")
	if len(s) > 0 && !strings.HasSuffix(s, "\n") {
		lines++
	}
	return lines, len(strings.Fields(s)), len(data), nil
}

// Try it with Run.
func main() {
	m, err := ParseKV(strings.NewReader("# config\nname = Ada\nlang=Go\n"))
	fmt.Println(m, err)
	fmt.Println(Stats(strings.NewReader("one two\nthree\n")))
}
