---
title: Find Orphans and Duplicate Owners
summary: Audit a block ownership graph without trusting inode pointers or bitmap state.
minutes: 40
difficulty: hard
id: go-filesystems/consistency-check
---
A consistency checker compares independent metadata views: the allocation bitmap and the inode ownership graph. A referenced but free block is dangling, an allocated but unreferenced data block is orphaned, and multiple references to one block mean duplicate ownership. A checker reports evidence before repair; automatically freeing a duplicate block can destroy another file. This exercise receives decoded metadata so it can focus on graph invariants.

Run the starter to inspect the public example, then implement the contract in the task. The reference implementation produces the specified results for these same inputs.
