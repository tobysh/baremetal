---
title: Pipelines of Any Length
summary: Generalise the two-command pipe to N commands with a loop that carries the previous read end forward, and learn which descriptors to close and when.
minutes: 40
difficulty: hard
tags: processes, pipes, shell
id: c-shell/pipeline
mode: harness
---
`cat log | grep error | sort | uniq -c | head` is five processes joined by four pipes. The two-command version does not scale if you write it out by hand, but the pattern repeats, so a loop can build it.

## The loop

Process the commands left to right. Keep one variable, `prev`, holding the **read end of the pipe that feeds the next command** (or the input file for the first one). For each command `i`:

1. If it is not the last command, create a new pipe `p`.
2. Fork. In the child:
   - `dup2(prev, 0)` if `prev` is a real descriptor;
   - `dup2(p[1], 1)` if there is a next pipe, or `dup2(out_fd, 1)` for the last command if `out_fd >= 0`;
   - close **every** descriptor the child no longer needs (`prev`, both ends of `p`, `out_fd`, `in_fd`), then `execvp`.
3. In the parent: close `prev` (if it is one of ours) and `p[1]`, then set `prev = p[0]`.

After the loop the parent holds no pipe ends. The parent's job each round is to hand the read end on and drop the write end immediately, so no process other than the writer ever keeps a write end alive.

## Why it all runs at once

You fork all the children **before** waiting for any of them. If you waited after each fork, `yes | head` would deadlock: `yes` fills the pipe buffer (about 64 KB) and blocks, waiting for a reader that is not started yet.

## Collecting the results

Remember every pid in an array. After the loop, wait for each one. The status of the pipeline is the status of the **last** command, as in `$?` in a real shell. A mid-pipeline failure (`false | true`) does not make the pipeline fail.

If `pipe` or `fork` fails half-way, close what you opened, wait for the children already started, and return `-1`.

## Your turn

Write `run_pipeline`. The array of commands is an array of `argv` arrays, so its type is `char ***`.
