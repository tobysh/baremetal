---
title: Order a Safe File Replacement
summary: Publish initialized data before freeing the previous file blocks.
minutes: 40
difficulty: medium
id: go-filesystems/ordered-replacement
---
Write ordering can preserve safety without providing all-or-nothing recovery. Reserve a fresh block, initialize it, publish the inode pointer, then release the old block. A crash before publication may leak the new block. A crash after publication may leak the old block. Neither case may expose uninitialized or free data. This is a one-block model of the replacement ordering used by WriteFile.

Run the starter to inspect the public example, then implement the contract in the task. The reference implementation produces the specified results for these same inputs.
