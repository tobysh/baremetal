---
title: Read a Fragmentation Map
summary: Render heap headers, allocated payloads and free payloads without changing heap state.
minutes: 40
difficulty: hard
id: c-allocators/fragmentation-map
---
Total free space can be misleading: a large request needs one contiguous hole. Draw the heap in 16-byte units, using H for each header, U for allocated payload units and . for free payload units. Header overhead and free gaps become visible separately. This is a representation of the teaching heap, not a process-wide memory profiler.
