package main

import (
	"errors"
	"fmt"
	"strconv"
	"strings"
)

// Member is one person in the book.
type Member struct {
	ID    int
	Name  string
	Email string
	Phone string // declared, but nothing reads or writes it yet
}

// Book keeps members in insertion order.
type Book struct {
	members []Member
}

// FormatV1 renders the version 1 line: id|name|email.
func FormatV1(m Member) string {
	return fmt.Sprintf("%d|%s|%s", m.ID, m.Name, m.Email)
}

// parseLine parses one record line.
func parseLine(line string) (Member, error) {
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

// isRecord reports whether a line holds a record (not blank, not a comment).
func isRecord(line string) bool {
	line = strings.TrimSpace(line)
	return line != "" && !strings.HasPrefix(line, "#")
}

// Load reads a book from stored data.
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

// Save renders the book for storage.
func (b *Book) Save() string {
	var sb strings.Builder
	for _, m := range b.members {
		sb.WriteString(FormatV1(m))
		sb.WriteByte('\n')
	}
	return sb.String()
}

// Add appends a member.
func (b *Book) Add(m Member) error {
	if m.ID <= 0 {
		return errors.New("bad id")
	}
	if strings.TrimSpace(m.Name) == "" {
		return errors.New("empty name")
	}
	for _, s := range []string{m.Name, m.Email} {
		if strings.ContainsAny(s, "|\r\n") {
			return errors.New("field contains a reserved character")
		}
	}
	if _, dup := b.Get(m.ID); dup {
		return fmt.Errorf("duplicate id %d", m.ID)
	}
	b.members = append(b.members, m)
	return nil
}

// Get finds a member by id.
func (b *Book) Get(id int) (Member, bool) {
	for _, m := range b.members {
		if m.ID == id {
			return m, true
		}
	}
	return Member{}, false
}

// Len is the number of members.
func (b *Book) Len() int { return len(b.members) }

// Migrate converts a stored file to the newest format.
func Migrate(data string) (string, error) {
	return data, nil
}

func main() {
	b, err := Load("# members\n1|Ada|ada@example.com\n2| Grace |grace@example.com\n")
	fmt.Println(b.Len(), err)
	fmt.Print(b.Save())
}
