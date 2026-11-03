Publication is the inode write; everything it exposes must already be durable.
===
Freeing the old block before publication creates a dangling reference at a crash cut.
