package main

import (
	"encoding/binary"
	"errors"
	"fmt"
	"io"
)

var ErrTooLarge = errors.New("payload too large")

func WriteFull(w io.Writer, p []byte) (int, error) {
	total, stalls := 0, 0
	for len(p) > 0 {
		n, err := w.Write(p)
		if n > 0 {
			if n > len(p) {
				n = len(p)
			}
			total += n
			p = p[n:]
			stalls = 0
		}
		if err != nil {
			return total, err
		}
		if n == 0 {
			stalls++
			if stalls >= 3 {
				return total, io.ErrShortWrite
			}
		}
	}
	return total, nil
}

func WriteFrame(w io.Writer, payload []byte) error {
	if len(payload) > 65535 {
		return ErrTooLarge
	}
	frame := make([]byte, 2, 2+len(payload))
	binary.BigEndian.PutUint16(frame, uint16(len(payload)))
	frame = append(frame, payload...)
	_, err := WriteFull(w, frame)
	return err
}

func ReadFrame(r io.Reader) ([]byte, error) {
	var hdr [2]byte
	if _, err := io.ReadFull(r, hdr[:]); err != nil {
		return nil, err
	}
	body := make([]byte, binary.BigEndian.Uint16(hdr[:]))
	if _, err := io.ReadFull(r, body); err != nil {
		if err == io.EOF {
			err = io.ErrUnexpectedEOF
		}
		return nil, err
	}
	return body, nil
}

func main() {
	fmt.Println("see tests")
}
