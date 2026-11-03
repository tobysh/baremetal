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

func inodeBlocks(n uint32) uint32 { return n/2 + n%2 }
func putU32(b []byte, v uint32)   { binary.BigEndian.PutUint32(b, v) }
func u32(b []byte) uint32         { return binary.BigEndian.Uint32(b) }
func FormatSuper(d Device, nblocks, ninodes uint32) error {
	if nblocks > 512 || uint64(nblocks) > uint64(d.Blocks()) || ninodes < 2 {
		return ErrBadSuper
	}
	start := 2 + inodeBlocks(ninodes)
	if start >= nblocks {
		return ErrNoSpace
	}
	b := make([]byte, 64)
	for i, v := range []uint32{0x424d4653, 1, nblocks, ninodes, 1, 2, start} {
		putU32(b[i*4:], v)
	}
	for i := uint32(0); i < start; i++ {
		d.WriteBlock(int(i), make([]byte, 64))
	}
	d.WriteBlock(0, b)
	return nil
}
func ReadSuper(d Device) (Super, error) {
	if d.Blocks() < 1 {
		return Super{}, ErrBadSuper
	}
	b := d.ReadBlock(0)
	s := Super{u32(b[8:]), u32(b[12:]), u32(b[16:]), u32(b[20:]), u32(b[24:])}
	if u32(b) != 0x424d4653 || u32(b[4:]) != 1 || s.NBlocks > 512 || uint64(s.NBlocks) > uint64(d.Blocks()) || s.NInodes < 2 || s.BitmapStart != 1 || s.InodeStart != 2 || s.DataStart != 2+inodeBlocks(s.NInodes) || s.DataStart >= s.NBlocks {
		return Super{}, ErrBadSuper
	}
	return s, nil
}
func GetBit(d Device, s Super, b int) (bool, error) {
	if b < 0 || b >= int(s.NBlocks) {
		return false, ErrRange
	}
	v := d.ReadBlock(int(s.BitmapStart))
	return v[b/8]&(1<<uint(b%8)) != 0, nil
}
func SetBit(d Device, s Super, b int, used bool) error {
	if b < 0 || b >= int(s.NBlocks) {
		return ErrRange
	}
	v := d.ReadBlock(int(s.BitmapStart))
	if used {
		v[b/8] |= 1 << uint(b%8)
	} else {
		v[b/8] &^= 1 << uint(b%8)
	}
	d.WriteBlock(int(s.BitmapStart), v)
	return nil
}
func MarkMetadata(d Device, s Super) {
	v := make([]byte, 64)
	for b := 0; b < int(s.DataStart); b++ {
		v[b/8] |= 1 << uint(b%8)
	}
	d.WriteBlock(int(s.BitmapStart), v)
}
func AllocBlock(d Device, s Super) (int, error) {
	for b := int(s.DataStart); b < int(s.NBlocks); b++ {
		used, _ := GetBit(d, s, b)
		if !used {
			SetBit(d, s, b, true)
			return b, nil
		}
	}
	return 0, ErrNoSpace
}
func FreeBlock(d Device, s Super, b int) error {
	if b < int(s.DataStart) || b >= int(s.NBlocks) {
		return ErrReserved
	}
	used, _ := GetBit(d, s, b)
	if !used {
		return ErrNotAllocated
	}
	return SetBit(d, s, b, false)
}
func CountFree(d Device, s Super) int {
	n := 0
	for b := int(s.DataStart); b < int(s.NBlocks); b++ {
		used, _ := GetBit(d, s, b)
		if !used {
			n++
		}
	}
	return n
}
func main() {
	d := NewDisk(32)
	FormatSuper(d, 32, 8)
	s, _ := ReadSuper(d)
	MarkMetadata(d, s)
	fmt.Println(AllocBlock(d, s))
	fmt.Println(CountFree(d, s))
}
