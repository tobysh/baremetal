---
title: An In-Memory Key-Value Store
summary: Start the database with the simplest thing that works, a map, and decide who owns the bytes you store.
minutes: 20
difficulty: easy
tags: maps, storage
id: go-database/kv-store
mode: harness
---
Every database is, at heart, a way to **put** a value under a key and **get** it back later. The simplest possible one is a Go map:

```go
data := map[string][]byte{}
data["user:1"] = []byte("ada")
v, ok := data["user:1"] // v = "ada", ok = true
```

Two details matter once other code starts using your store.

## Missing is not empty

A key that holds an empty value is different from a key that was never stored. Always report presence separately with the `ok` result, never by testing `len(v) == 0`.

## Who owns the bytes?

A `[]byte` is a view onto an array that other code may still hold. If the caller stores a slice and later reuses its buffer, the value in your store silently changes:

```go
buf := []byte("hello")
store.Put("k", buf)
copy(buf, "XXXXX") // the store must still say "hello"
```

A database must not let that happen, so `Put` stores a **copy**. The same goes for `Get`: if it hands out the internal slice, a caller who edits the result edits the database. Copy on the way in and on the way out.

## Listing keys

Go randomises map iteration order on purpose. A database that lists its keys needs a stable order, so collect them and `sort.Strings` them. Sorted order will matter a great deal in later lessons.
