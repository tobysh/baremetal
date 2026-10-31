package main

import (
	"errors"
	"fmt"
	"regexp"
	"strconv"
	"strings"
)

type Member struct {
	ID    int
	Name  string
	Email string
	Phone string
}

type Book struct {
	members []Member
}

func esc(s string) string {
	var b strings.Builder
	for _, r := range s {
		switch r {
		case '\\':
			b.WriteString(`\\`)
		case '|':
			b.WriteString(`\|`)
		case '\n':
			b.WriteString(`\n`)
		case '\r':
			b.WriteString(`\r`)
		default:
			b.WriteRune(r)
		}
	}
	return b.String()
}

// FormatV2 renders the version 2 line.
func FormatV2(m Member) string {
	return fmt.Sprintf("v2|%d|%s|%s|%s", m.ID, esc(m.Name), esc(m.Email), esc(m.Phone))
}

func splitV2(s string) ([]string, error) {
	var fields []string
	var cur strings.Builder
	rs := []rune(s)
	for i := 0; i < len(rs); i++ {
		r := rs[i]
		switch r {
		case '\\':
			i++
			if i >= len(rs) {
				return nil, errors.New("trailing backslash")
			}
			switch rs[i] {
			case '\\', '|':
				cur.WriteRune(rs[i])
			case 'n':
				cur.WriteByte('\n')
			case 'r':
				cur.WriteByte('\r')
			default:
				return nil, fmt.Errorf("bad escape \\%c", rs[i])
			}
		case '|':
			fields = append(fields, cur.String())
			cur.Reset()
		default:
			cur.WriteRune(r)
		}
	}
	return append(fields, cur.String()), nil
}

var verRe = regexp.MustCompile(`^(v[0-9]+)\|`)

func parseV1(line string) (Member, error) {
	parts := strings.Split(line, "|")
	if len(parts) != 3 {
		return Member{}, errors.New("want 3 fields")
	}
	id, err := strconv.Atoi(strings.TrimSpace(parts[0]))
	if err != nil || id <= 0 {
		return Member{}, errors.New("bad id")
	}
	name := strings.TrimSpace(parts[1])
	if name == "" {
		return Member{}, errors.New("empty name")
	}
	return Member{ID: id, Name: name, Email: strings.TrimSpace(parts[2])}, nil
}

func parseLine(line string) (Member, error) {
	if m := verRe.FindStringSubmatch(line); m != nil {
		if m[1] != "v2" {
			return Member{}, fmt.Errorf("unsupported version %q", m[1])
		}
		f, err := splitV2(line[3:])
		if err != nil {
			return Member{}, err
		}
		if len(f) != 4 {
			return Member{}, errors.New("want 4 fields after v2")
		}
		id, err := strconv.Atoi(f[0])
		if err != nil || id <= 0 {
			return Member{}, errors.New("bad id")
		}
		if f[1] == "" {
			return Member{}, errors.New("empty name")
		}
		return Member{ID: id, Name: f[1], Email: f[2], Phone: f[3]}, nil
	}
	return parseV1(line)
}

func isRecord(line string) bool {
	line = strings.TrimSpace(line)
	return line != "" && !strings.HasPrefix(line, "#")
}

func Load(data string) (*Book, error) {
	b := &Book{}
	for i, line := range strings.Split(data, "\n") {
		line = strings.TrimSuffix(line, "\r")
		if !isRecord(line) {
			continue
		}
		m, err := parseLine(line)
		if err != nil {
			return nil, fmt.Errorf("line %d: %v", i+1, err)
		}
		if _, dup := b.Get(m.ID); dup {
			return nil, fmt.Errorf("line %d: duplicate id %d", i+1, m.ID)
		}
		b.members = append(b.members, m)
	}
	return b, nil
}

func (b *Book) Save() string {
	var sb strings.Builder
	for _, m := range b.members {
		sb.WriteString(FormatV2(m))
		sb.WriteByte('\n')
	}
	return sb.String()
}

func (b *Book) Add(m Member) error {
	if m.ID <= 0 {
		return errors.New("bad id")
	}
	if strings.TrimSpace(m.Name) == "" {
		return errors.New("empty name")
	}
	if _, dup := b.Get(m.ID); dup {
		return fmt.Errorf("duplicate id %d", m.ID)
	}
	b.members = append(b.members, m)
	return nil
}

func (b *Book) Get(id int) (Member, bool) {
	for _, m := range b.members {
		if m.ID == id {
			return m, true
		}
	}
	return Member{}, false
}

func (b *Book) Len() int { return len(b.members) }

func Migrate(data string) (string, error) {
	lines := strings.Split(data, "\n")
	out := make([]string, 0, len(lines))
	for i, line := range lines {
		line = strings.TrimSuffix(line, "\r")
		if !isRecord(line) {
			out = append(out, line)
			continue
		}
		m, err := parseLine(line)
		if err != nil {
			return "", fmt.Errorf("line %d: %v", i+1, err)
		}
		out = append(out, FormatV2(m))
	}
	return strings.Join(out, "\n"), nil
}

func main() {
	b, err := Load("# members\n1|Ada|ada@example.com\n2| Grace |grace@example.com\n")
	fmt.Println(b.Len(), err)
	fmt.Print(b.Save())
}
