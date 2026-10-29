---
title: A Bounded Pipe
summary: Move bytes through a circular pipe buffer and distinguish empty from EOF.
minutes: 35
difficulty: medium
tags: kernel, systems
id: osdev-2/pipes
mode: harness
---
A pipe connects a writer to a reader through a bounded FIFO. A circular buffer avoids shifting data on every read. An empty pipe with a writer still open means “try again”; an empty pipe after its writer closes means end of file. Here operations are nonblocking so the scheduler is not needed. A blocking implementation would sleep and wake under the same lock that protects the buffer.

Writes and reads may transfer fewer bytes than requested. Closing the writer does not discard buffered bytes: the reader receives those before EOF.
