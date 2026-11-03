package main

import "fmt"
import (
	"encoding/binary"
	"errors"
)

const BlockSize = 64
const InodeSize = 32
const InodesPerBlock = 2
const MaxFile = 384

var ErrBadSuper = errors.New("bad superblock")
var ErrNoSpace = errors.New("no space")
var ErrRange = errors.New("range")
var ErrReserved = errors.New("reserved")
var ErrNotAllocated = errors.New("not allocated")
var ErrNotFound = errors.New("not found")
var ErrTooBig = errors.New("too big")

type Device interface {
	ReadBlock(int) []byte
	WriteBlock(int, []byte)
	Blocks() int
}
type MemDisk struct {
	Data   [][]byte
	Writes int
}

func NewDisk(n int) *MemDisk {
	d := &MemDisk{Data: make([][]byte, n)}
	for i := range d.Data {
		d.Data[i] = make([]byte, 64)
	}
	return d
}
func (d *MemDisk) Blocks() int                { return len(d.Data) }
func (d *MemDisk) ReadBlock(b int) []byte     { return append([]byte(nil), d.Data[b]...) }
func (d *MemDisk) WriteBlock(b int, v []byte) { copy(d.Data[b], v); d.Writes++ }

// Super describes 64-byte BMFS blocks: block 0 is the superblock, block 1
// is the bitmap, and inode storage begins at block 2. Two inodes fit per block.
type Super struct{ NBlocks, NInodes, BitmapStart, InodeStart, DataStart uint32 }

func inodeBlocks(n uint32) uint32                         { return n/2 + n%2 }
func putU32(b []byte, v uint32)                           { binary.BigEndian.PutUint32(b, v) }
func u32(b []byte) uint32                                 { return binary.BigEndian.Uint32(b) }
func FormatSuper(d Device, nblocks, ninodes uint32) error { return nil }
func ReadSuper(d Device) (Super, error)                   { return Super{}, nil }
func main()                                               { d := NewDisk(32); fmt.Println(FormatSuper(d, 32, 8)); fmt.Println(ReadSuper(d)) }
