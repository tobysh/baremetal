---
title: Directories Name Inodes
summary: Encode bounded directory records and reject ambiguous or malformed names.
minutes: 40
difficulty: hard
tags: filesystem, on-disk-format, directories
id: go-filesystems/directories
---
A file's inode says what the file contains but not what it is called. The name lives one level up, in a **directory**: a file whose contents are a list of `(name, inode number)` records. Opening `notes` means reading the directory, finding the record named `notes`, and following its inode number. Because names are in the directory and not in the inode, two records can point at the same inode (hard links, which is what the inode's link count counts), and renaming a file only rewrites a directory.

## The wire format

This checkpoint isolates just the encoding. A directory's bytes are a sequence of fixed 16-byte records:

```text
bytes 0..11   name, ASCII, followed by zero padding up to 12 bytes
bytes 12..15  inode number, big-endian, must be > 0
```

A directory with `{"a",1}` and `{"twelve-chars",19}` is 32 bytes:

```text
61 00 00 00 00 00 00 00 00 00 00 00  00 00 00 01
74 77 65 6c 76 65 2d 63 68 61 72 73  00 00 00 13
```

A fixed record size means the n-th entry is at offset `16*n` and an empty directory is zero bytes. A 12-byte name uses all its bytes and has no terminating NUL.

## Rules a name must obey

- 1 to 12 bytes, every byte in `1..127`: so no NUL, no `/` (the path separator), nothing outside ASCII;
- unique within the directory (names are case sensitive, `A` and `a` differ);
- the inode number is positive, because inode 0 is never allocated and would read as "no entry".

Dots are ordinary characters here: `.hidden` and `a.b` are valid names, and `.` and `..` navigation belongs to a path layer outside this model.

## Decoding is parsing untrusted bytes

A directory read from disk is input you do not control, and the dangerous bugs are in what you accept. Two byte patterns decode to the same name in a sloppy parser: `"a\0\0..."` and `"a\0x\0..."` if you stop at the first NUL and ignore the rest. That gives a name two spellings, and a duplicate check on the string cannot see them. So the **padding is part of the format**: after the first NUL every remaining name byte must be zero. Equally, a length that is not a multiple of 16 is truncation, not a short last record.

```go
end := bytes.IndexByte(name, 0) // -1 if the name fills all 12 bytes
if end < 0 {
	end = len(name)
}
for _, b := range name[end:] { // must all be zero
	...
}
```

`Encode` must apply the same validation as `Decode`, so the two are inverses on valid input and neither can produce what the other rejects. Validate everything before returning anything: on error the output is `nil`, never a half-built slice.

## Limits of the model

A real directory also stores a type per entry, grows across blocks, and needs `.` and `..`, deletion and rename. Here a directory is a plain slice of records with no capacity limit, which keeps the focus on the format.
