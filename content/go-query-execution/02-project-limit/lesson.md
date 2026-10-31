---
title: Project and Limit
summary: Compute output columns with NULL-propagating expressions, then stop a pipeline early by refusing to pull more rows than a LIMIT needs.
minutes: 35
difficulty: medium
tags: iterator, projection, limit, laziness
id: go-query-execution/project-limit
mode: harness
---
**Project** turns each input row into a new row by evaluating one expression per output column. Expressions follow the NULL rule from the previous lesson: `NULL + 1` is NULL, and so is NULL concatenated with text.

**Limit** shows why the pull model matters. `LIMIT 3` should read three rows of a million-row table, not a million. Because nothing runs until the parent calls `Next`, Limit only has to stop calling. Two details are easy to get wrong:

- After Limit has returned its last row, a further `Next` must not touch the child. One extra pull reads (and pays for) a row nobody asked for.
- `OFFSET` rows are pulled and thrown away, so they do count as work.

Project must also build a *new* `Row` each time. If it returned a slice of the input row, a caller editing a result would edit the table.

The filter and scan are provided. The tests check the exact number of rows read through a Filter, Project and Limit pipeline.
