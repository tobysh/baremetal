---
title: Query Capstone
summary: Capstone. Turn a table of 64-byte rows into columns sorted by region in one allocation, and answer region queries with a fraction of the cache misses.
minutes: 50
difficulty: hard
tags: performance, capstone, layout, allocation, indexing
id: c-performance/query-capstone
mode: harness
---
This capstone combines the course: **layout** (lesson 3), **allocation** (lesson 4), an **access-order or index** choice (lessons 1, 2 and 5), and **evidence** (all of them).

## The task

An orders table has `n` rows of 64 bytes:

```c
typedef struct { int id, region, price, qty; int pad[12]; } row_t;
```

The query is: for region `r` and a price floor `min_price`, return the sum of `price * qty` over rows with `region == r` and `price >= min_price`, as a `long`.

`query_aos` reads the rows directly. For every row it reads `region` (and `price`, `qty` when needed). Each row is its own cache line, so it costs **one miss per row**, matched or not: 2000 rows, 2000 misses.

## The faster design

Build a `table_t` once:

- columns `price[]` and `qty[]` (4 bytes per row), **stable-sorted by region** so each region occupies a contiguous range;
- `start[0..REGIONS]`, where region `r` owns indices `start[r]` up to but not including `start[r+1]`;
- all of it in **one** `sys_alloc` block: `price` at offset 0, `qty` at `round64(4n)`, `start` at `2 * round64(4n)`, total `2 * round64(4n) + round64(4 * (REGIONS + 1))` bytes (`round64` rounds up to a multiple of 64).

A query then reads `start[r]` and `start[r+1]`, and walks only the `k` rows of that region through `price[]` (and `qty[]` when the price qualifies): about `k/16` lines per column. For `k = 250` that is under 40 misses against 2000.

Build uses a counting sort: count rows per region, prefix-sum into `start`, then place each row. Building reads every row once and is **not** charged to the cache model; it is setup. That is a real tradeoff, stated rather than hidden: the layout only pays if the table is built once and queried many times. If the data changes between queries, rebuild costs would have to be counted. The tests do not weigh this, but a good report would.

## What is checked

- Results equal a plain reference loop for many tables: empty, one row, all rows in one region, empty regions, price floors of 0 and above the maximum, and regions outside `0..REGIONS-1` (which return 0 and must not read outside `start[]`).
- Exactly one `sys_alloc` per table of at most the stated size; `table_free` returns `sys_live` to 0.
- Misses per query stay under `2 * (k/16 + 2) + 2`, and for a large table are at least 8 times fewer than `query_aos`.

## Limits

One cold-cache query is measured; a stream of queries would warm the cache and shrink both numbers. The model has no prefetcher, and no hardware timing is involved. An index by region helps a query that names a region. A query that scans all rows would favour columns without the sorting. Justifying which design to ship needs that workload information, which only you can supply.
