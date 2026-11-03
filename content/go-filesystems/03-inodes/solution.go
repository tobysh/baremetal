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

type Inode struct {
	Type, Links byte
	Size        uint32
	Direct      [6]uint32
}

func (i Inode) blocks() []uint32         { return i.Direct[:(i.Size+63)/64] }
func inodePos(s Super, n int) (int, int) { return int(s.InodeStart) + n/2, (n % 2) * 32 }
func ReadInode(d Device, s Super, n int) (Inode, error) {
	if n < 0 || n >= int(s.NInodes) {
		return Inode{}, ErrRange
	}
	b, o := inodePos(s, n)
	v := d.ReadBlock(b)[o:]
	in := Inode{Type: v[0], Links: v[1], Size: u32(v[4:])}
	for j := range in.Direct {
		in.Direct[j] = u32(v[8+j*4:])
	}
	return in, nil
}
func WriteInode(d Device, s Super, n int, in Inode) error {
	if n < 0 || n >= int(s.NInodes) {
		return ErrRange
	}
	b, o := inodePos(s, n)
	v := d.ReadBlock(b)
	clear(v[o : o+32])
	v[o] = in.Type
	v[o+1] = in.Links
	putU32(v[o+4:], in.Size)
	for j, x := range in.Direct {
		putU32(v[o+8+j*4:], x)
	}
	d.WriteBlock(b, v)
	return nil
}
func AllocInode(d Device, s Super, kind byte) (int, error) {
	for n := 1; n < int(s.NInodes); n++ {
		in, _ := ReadInode(d, s, n)
		if in.Type == 0 {
			WriteInode(d, s, n, Inode{Type: kind, Links: 1})
			return n, nil
		}
	}
	return 0, ErrNoSpace
}
func ReadFile(d Device, s Super, n int) ([]byte, error) {
	in, e := ReadInode(d, s, n)
	if e != nil {
		return nil, e
	}
	if in.Type == 0 {
		return nil, ErrNotFound
	}
	out := []byte{}
	for _, b := range in.blocks() {
		out = append(out, d.ReadBlock(int(b))...)
	}
	return out[:in.Size], nil
}
func writeData(d Device, b int, v []byte) { d.WriteBlock(b, v) }
func WriteFile(d Device, s Super, n int, data []byte) error {
	in, e := ReadInode(d, s, n)
	if e != nil {
		return e
	}
	if in.Type == 0 {
		return ErrNotFound
	}
	if len(data) > MaxFile {
		return ErrTooBig
	}
	count := (len(data) + 63) / 64
	if CountFree(d, s) < count {
		return ErrNoSpace
	}
	old := in
	in.Direct = [6]uint32{}
	for j := 0; j < count; j++ {
		b, _ := AllocBlock(d, s)
		in.Direct[j] = uint32(b)
	}
	for j := 0; j < count; j++ {
		v := make([]byte, 64)
		copy(v, data[j*64:min(len(data), (j+1)*64)])
		writeData(d, int(in.Direct[j]), v)
	}
	in.Size = uint32(len(data))
	WriteInode(d, s, n, in)
	for _, b := range old.blocks() {
		FreeBlock(d, s, int(b))
	}
	return nil
}
func FreeInode(d Device, s Super, n int) error {
	in, e := ReadInode(d, s, n)
	if e != nil {
		return e
	}
	if in.Type == 0 {
		return ErrNotFound
	}
	for _, b := range in.blocks() {
		FreeBlock(d, s, int(b))
	}
	return WriteInode(d, s, n, Inode{})
}
func main() {
	d := NewDisk(32)
	FormatSuper(d, 32, 8)
	s, _ := ReadSuper(d)
	MarkMetadata(d, s)
	n, _ := AllocInode(d, s, 1)
	fmt.Println(WriteFile(d, s, n, []byte("hello")))
	v, e := ReadFile(d, s, n)
	fmt.Println(string(v), e)
}
