package main

import (
	"encoding/binary"
	"fmt"
	"strings"
)

type Config struct {
	Label    string
	Interval uint16
	MaxOps   uint16
	Allow    []string
	Token    string
}

func Decode(b []byte) (Config, error) {
	fail := func(s string) (Config, error) { return Config{}, fmt.Errorf("config: %s", s) }
	if len(b) < 7 || string(b[:4]) != "BCFG" {
		return fail("header")
	}
	ver := b[4]
	if ver != 1 && ver != 2 {
		return fail("version")
	}
	n := int(binary.LittleEndian.Uint16(b[5:7]))
	if n > 1024 || len(b) != 7+n {
		return fail("length")
	}
	body := b[7:]
	c := Config{Interval: 10, MaxOps: 64, Allow: []string{}}
	seen := map[byte]bool{}
	for i := 0; i < len(body); {
		if len(body)-i < 2 {
			return fail("truncated field")
		}
		tag, l := body[i], int(body[i+1])
		i += 2
		if l > len(body)-i {
			return fail("field overruns body")
		}
		v := body[i : i+l]
		i += l
		known := tag >= 1 && tag <= 3 || (ver >= 2 && (tag == 4 || tag == 5))
		if !known {
			if tag < 0x80 {
				return fail("unknown critical tag")
			}
			continue
		}
		if seen[tag] {
			return fail("duplicate tag")
		}
		seen[tag] = true
		switch tag {
		case 1:
			if l < 1 || l > 32 {
				return fail("label")
			}
			c.Label = string(v)
		case 2:
			if l != 2 || v[0]|v[1] == 0 {
				return fail("interval")
			}
			c.Interval = binary.LittleEndian.Uint16(v)
		case 3:
			m := binary.LittleEndian.Uint16(append(v, 0, 0)[:2])
			if l != 2 || m < 1 || m > 4096 {
				return fail("maxops")
			}
			c.MaxOps = m
		case 4:
			if l < 1 || v[0] > 8 {
				return fail("allow count")
			}
			cnt, j := int(v[0]), 1
			keys := map[string]bool{}
			for k := 0; k < cnt; k++ {
				if j >= l {
					return fail("allow truncated")
				}
				kl := int(v[j])
				j++
				if kl < 1 || kl > 32 || kl > l-j {
					return fail("allow key")
				}
				key := string(v[j : j+kl])
				j += kl
				if keys[key] {
					return fail("allow duplicate")
				}
				keys[key] = true
				c.Allow = append(c.Allow, key)
			}
			if j != l {
				return fail("allow trailing")
			}
		case 5:
			if l < 1 || l > 64 {
				return fail("token")
			}
			c.Token = string(v)
		}
	}
	if !seen[1] {
		return fail("missing label")
	}
	return c, nil
}

func Encode(c Config) ([]byte, error) {
	fail := func(s string) ([]byte, error) { return nil, fmt.Errorf("config: %s", s) }
	if len(c.Label) < 1 || len(c.Label) > 32 || c.Interval == 0 || c.MaxOps < 1 || c.MaxOps > 4096 || len(c.Allow) > 8 || len(c.Token) > 64 {
		return fail("invalid value")
	}
	seen := map[string]bool{}
	for _, k := range c.Allow {
		if len(k) < 1 || len(k) > 32 || seen[k] {
			return fail("invalid allow list")
		}
		seen[k] = true
	}
	ver := byte(1)
	if len(c.Allow) > 0 || c.Token != "" {
		ver = 2
	}
	body := []byte{1, byte(len(c.Label))}
	body = append(body, c.Label...)
	if c.Interval != 10 {
		body = append(body, 2, 2, byte(c.Interval), byte(c.Interval>>8))
	}
	if c.MaxOps != 64 {
		body = append(body, 3, 2, byte(c.MaxOps), byte(c.MaxOps>>8))
	}
	if len(c.Allow) > 0 {
		v := []byte{byte(len(c.Allow))}
		for _, k := range c.Allow {
			v = append(v, byte(len(k)))
			v = append(v, k...)
		}
		body = append(body, 4, byte(len(v)))
		body = append(body, v...)
	}
	if c.Token != "" {
		body = append(body, 5, byte(len(c.Token)))
		body = append(body, c.Token...)
	}
	out := []byte{'B', 'C', 'F', 'G', ver, byte(len(body)), byte(len(body) >> 8)}
	return append(out, body...), nil
}

func Describe(c Config) string {
	tok := "unset"
	if c.Token != "" {
		tok = fmt.Sprintf("set(len=%d)", len(c.Token))
	}
	return fmt.Sprintf("label=%s interval=%d maxops=%d allow=%s token=%s", c.Label, c.Interval, c.MaxOps, strings.Join(c.Allow, ","), tok)
}

func main() {
	c := Config{Label: "lab", Interval: 10, MaxOps: 64, Allow: []string{"a", "b"}, Token: "lab-token-1"}
	b, err := Encode(c)
	fmt.Printf("% x (%v)\n", b, err)
	d, err := Decode(b)
	fmt.Println(Describe(d), err)
}
