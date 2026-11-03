---
title: Capstone: Route Small Allocations
summary: Combine bounded small-object pools with a general heap and route frees by ownership.
minutes: 40
difficulty: hard
id: c-allocators/size-class-allocator
---
Small fixed-size objects can avoid general-heap searches by using dedicated pools. This capstone has eight 16-byte slots and eight 64-byte slots, plus the supplied general heap. Requests up to 16 use the small pool, up to 64 use the larger pool, and larger requests use the heap. A full preferred pool falls back to the heap. Static pools are outside the heap buffer and all returned addresses are 16-byte aligned. This is a single-threaded model: it omits concurrency, realloc and general malloc compatibility.
