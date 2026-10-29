package main

import (
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"
	"io"
)

const MaxField = 1 << 20

var ErrTooLarge = errors.New("record field too large")

func AppendRecord(w io.Writer, key, value []byte) error {
	if len(key) > MaxField || len(value) > MaxField {
		return ErrTooLarge
	}
	rec := make([]byte, 8+len(key)+len(value))
	binary.BigEndian.PutUint32(rec[0:4], uint32(len(key)))
	binary.BigEndian.PutUint32(rec[4:8], uint32(len(value)))
	copy(rec[8:], key)
	copy(rec[8+len(key):], value)
	_, err := w.Write(rec)
	return err
}

func ReadRecord(r io.Reader) (key, value []byte, err error) {
	var hdr [8]byte
	if _, err := io.ReadFull(r, hdr[:]); err != nil {
		return nil, nil, err
	}
	kl := binary.BigEndian.Uint32(hdr[0:4])
	vl := binary.BigEndian.Uint32(hdr[4:8])
	if kl > MaxField || vl > MaxField {
		return nil, nil, ErrTooLarge
	}
	body := make([]byte, kl+vl)
	if _, err := io.ReadFull(r, body); err != nil {
		if err == io.EOF {
			err = io.ErrUnexpectedEOF
		}
		return nil, nil, err
	}
	return body[:kl:kl], body[kl:], nil
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
