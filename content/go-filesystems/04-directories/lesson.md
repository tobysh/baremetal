---
title: Directories Name Inodes
summary: Encode bounded directory records and reject ambiguous or malformed names.
minutes: 40
difficulty: hard
id: go-filesystems/directories
---
A directory maps names to inode numbers. Names live in directory data rather than in the inode itself, allowing several names to refer to the same inode. This checkpoint isolates the directory wire format: each 16-byte record has a 12-byte zero-padded ASCII name and a big-endian inode number. A name is 1–12 bytes, contains no slash or NUL, and is unique. An inode number must be positive. Dot entries and paths are outside this model.

Run the starter to inspect the public example, then implement the contract in the task. The reference implementation produces the specified results for these same inputs.
