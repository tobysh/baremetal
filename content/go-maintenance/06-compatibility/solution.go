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

// Parse reads "key = value" lines. It is the legacy entry point and never
// treats quotes specially.
func Parse(src string) (*Config, error) { return ParseOpts(src, Options{}) }

// Options selects optional parsing behaviour. The zero value is legacy.
type Options struct {
	Quotes bool
}

// ParseOpts is Parse with options.
func ParseOpts(src string, o Options) (*Config, error) {
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
		if o.Quotes && strings.HasPrefix(val, `"`) {
			q, err := readQuoted(val)
			if err != nil {
				return nil, fmt.Errorf("line %d: %v", i+1, err)
			}
			c.set(key, q)
			continue
		}
		val = stripComment(val)
		c.set(key, val)
	}
	return c, nil
}

func readQuoted(v string) (string, error) {
	var b strings.Builder
	rs := []rune(v)
	for i := 1; i < len(rs); i++ {
		switch rs[i] {
		case '\\':
			i++
			if i >= len(rs) {
				return "", fmt.Errorf("unterminated quote")
			}
			switch rs[i] {
			case '"', '\\':
				b.WriteRune(rs[i])
			case 'n':
				b.WriteByte('\n')
			default:
				return "", fmt.Errorf("bad escape")
			}
		case '"':
			rest := string(rs[i+1:])
			t := strings.TrimLeft(rest, " ")
			if t != "" && !(strings.HasPrefix(t, "#") && len(t) != len(rest)) {
				return "", fmt.Errorf("junk after quoted value")
			}
			return b.String(), nil
		default:
			b.WriteRune(rs[i])
		}
	}
	return "", fmt.Errorf("unterminated quote")
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

// Bool reads a boolean value, falling back to def.
func (c *Config) Bool(key string, def bool) bool {
	switch strings.ToLower(strings.TrimSpace(c.vals[key])) {
	case "true", "yes", "on", "1":
		return true
	case "false", "no", "off", "0":
		return false
	}
	return def
}

func main() {
	c, err := Parse("name = demo # the name\nport = 8080\nmsg = \"hi\"\n")
	fmt.Println(err, c.Keys(), c.Get("name"), c.Int("port", 0), c.Get("msg"))
}
