package main

import (
	"fmt"
	"strconv"
	"strings"
)

// Config is a parsed key/value file.
type Config struct {
	vals  map[string]string
	order []string
}

// Parse reads "key = value" lines.
func Parse(src string) (*Config, error) {
	c := &Config{vals: map[string]string{}}
	for i, line := range strings.Split(src, "\n") {
		line = strings.TrimSpace(line)
		if line == "" || line[0] == '#' || line[0] == ';' {
			continue
		}
		eq := strings.IndexByte(line, '=')
		if eq < 0 {
			return nil, fmt.Errorf("line %d: missing '='", i+1)
		}
		key := strings.TrimSpace(line[:eq])
		if key == "" {
			return nil, fmt.Errorf("line %d: empty key", i+1)
		}
		val := strings.TrimSpace(line[eq+1:])
		val = stripComment(val)
		c.set(key, val)
	}
	return c, nil
}

// stripComment removes a trailing " #..." comment (a # at the start of the
// value, or one preceded by a space).
func stripComment(v string) string {
	for i := 0; i < len(v); i++ {
		if v[i] == '#' && (i == 0 || v[i-1] == ' ') {
			return strings.TrimSpace(v[:i])
		}
	}
	return v
}

func (c *Config) set(key, val string) {
	if _, ok := c.vals[key]; !ok {
		c.order = append(c.order, key)
	}
	c.vals[key] = val
}

// Get returns the value for key, or "" when missing.
func (c *Config) Get(key string) string { return c.vals[key] }

// Has reports whether key was set.
func (c *Config) Has(key string) bool {
	_, ok := c.vals[key]
	return ok
}

// Keys lists keys in the order they first appeared.
func (c *Config) Keys() []string { return append([]string(nil), c.order...) }

// Int parses the value as an integer, falling back to def.
func (c *Config) Int(key string, def int) int {
	n, err := strconv.Atoi(c.vals[key])
	if err != nil {
		return def
	}
	return n
}

// Options selects optional parsing behaviour. The zero value must stay legacy.
type Options struct {
	Quotes bool
}

// ParseOpts is Parse with options. TODO: implement Quotes.
func ParseOpts(src string, o Options) (*Config, error) {
	return Parse(src)
}

// Bool reads a boolean value, falling back to def. TODO.
func (c *Config) Bool(key string, def bool) bool {
	return def
}

func main() {
	c, err := Parse("name = demo # the name\nport = 8080\nmsg = \"hi\"\n")
	fmt.Println(err, c.Keys(), c.Get("name"), c.Int("port", 0), c.Get("msg"))
}
