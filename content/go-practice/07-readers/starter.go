package main

import (
	"fmt"
	"io"
	"strings"
)

func ParseKV(r io.Reader) (map[string]string, error) {
	return nil, nil
}

func Stats(r io.Reader) (lines, words, bytes int, err error) {
	return 0, 0, 0, nil
}

// Try it with Run.
func main() {
	m, err := ParseKV(strings.NewReader("# config\nname = Ada\nlang=Go\n"))
	fmt.Println(m, err)
	fmt.Println(Stats(strings.NewReader("one two\nthree\n")))
}
