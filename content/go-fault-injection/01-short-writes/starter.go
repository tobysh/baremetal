package main

import (
	"encoding/binary"
	"errors"
	"fmt"
	"io"
)

var ErrTooLarge = errors.New("payload too large")

// WriteFull must write all of p, continuing after short writes.
func WriteFull(w io.Writer, p []byte) (int, error) {
	return w.Write(p)
}

// WriteFrame writes a 2-byte big-endian length followed by the payload.
func WriteFrame(w io.Writer, payload []byte) error {
	frame := make([]byte, 2, 2+len(payload))
	binary.BigEndian.PutUint16(frame, uint16(len(payload)))
	frame = append(frame, payload...)
	_, err := WriteFull(w, frame)
	return err
}

// ReadFrame reads one frame written by WriteFrame.
func ReadFrame(r io.Reader) ([]byte, error) {
	buf := make([]byte, 65537)
	n, err := r.Read(buf)
	if err != nil {
		return nil, err
	}
	l := int(binary.BigEndian.Uint16(buf))
	return buf[2 : 2+l], nil
}

// chunkWriter accepts at most 4 bytes per call, silently.
type chunkWriter struct{ got []byte }

func (c *chunkWriter) Write(p []byte) (int, error) {
	if len(p) > 4 {
		p = p[:4]
	}
	c.got = append(c.got, p...)
	return len(p), nil
}

func main() {
	var w chunkWriter
	n, err := WriteFull(&w, []byte("hello, fault injection"))
	fmt.Printf("accepted %d of 22 bytes, err=%v, sink=%q\n", n, err, w.got)
}
