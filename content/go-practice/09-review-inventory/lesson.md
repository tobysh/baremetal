---
title: "Review: Inventory"
summary: A closing consolidation exercise: generics, wrapped errors, sorting, pointer receivers, embedding and JSON tags in one small stock system.
minutes: 30
difficulty: medium
kind: review
tags: generics, errors, sorting, embedding, json
id: go-practice/review-inventory
mode: harness
---
This is a **review lesson**. It introduces nothing new: every piece below comes from an earlier lesson in this track, and the goal is to see them work together.

Lessons this review draws on: [Generics](/learn/go-practice/generics), [Wrapping and Inspecting Errors](/learn/go-practice/error-wrapping), [Sorting and Closures](/learn/go-practice/sorting-and-closures), [Value vs pointer receivers](/learn/go-practice/value-vs-pointer-receivers), [Embedding and composition](/learn/go-practice/embedding), [JSON with Struct Tags](/learn/go-practice/json).

## What you already know

- **Generics:** `func Filter[T any](xs []T, keep func(T) bool) []T` works for any element type.
- **Wrapped errors:** a sentinel made with `errors.New`, wrapped with `fmt.Errorf("...: %w", err)`, found again by `errors.Is`.
- **Sorting with closures:** `sort.Slice(xs, func(i, j int) bool { ... })`, or `sort.SliceStable` when equal elements must keep their order. A function can be a parameter that decides the order.
- **Pointer receivers:** a method that changes a `Store` needs `func (s *Store)`.
- **Embedding:** a struct that embeds another gets its methods promoted, and can shadow one and call the inner version explicitly.
- **JSON tags:** `json:"name"` renames a field and `omitempty` drops a zero value.

## The program

A `Store` keeps `Item`s by SKU. Prices are whole cents. Adding an item that already exists adds to its quantity. Removing stock can fail in two different ways, and the caller must be able to tell which with `errors.Is`. `Sorted` returns the items in an order chosen by the caller's `less` function, falling back to SKU order for ties so the result is predictable. `Where` uses your generic `Filter`. `LoggedStore` embeds a `Store` and records what happened when stock is removed, while `Add` is simply promoted.
