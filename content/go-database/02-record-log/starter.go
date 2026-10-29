package main

import (
	"bytes"
	"errors"
	"fmt"
	"io"
)

const MaxField = 1 << 20

var ErrTooLarge = errors.New("record field too large")

func AppendRecord(w io.Writer, key, value []byte) error {
	return nil
}

func ReadRecord(r io.Reader) (key, value []byte, err error) {
	return nil, nil, io.EOF
}

// Try it with Run.
func main() {
	var buf bytes.Buffer
	AppendRecord(&buf, []byte("name"), []byte("ada"))
	AppendRecord(&buf, []byte("lang"), []byte("go"))
	fmt.Printf("% x\n", buf.Bytes())
	for {
		k, v, err := ReadRecord(&buf)
		if err != nil {
			fmt.Println(err)
			return
		}
		fmt.Printf("%s=%s\n", k, v)
	}
}
