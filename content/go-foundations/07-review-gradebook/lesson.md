---
title: "Review: Gradebook"
summary: A consolidation exercise. Combine structs, slices, maps, an interface and error wrapping in a tiny gradebook. No new ideas, just practice.
minutes: 25
difficulty: medium
kind: review
tags: structs, interfaces, errors, maps
id: go-foundations/review-gradebook
mode: harness
---
This is a **review lesson**. It introduces nothing new. You have covered functions with several return values, slices and maps, structs with methods, interfaces and errors. Here they all appear in one small program.

## What you already know

- **Structs and methods:** `type Student struct{ ... }` with a method that reads its fields.
- **Slices and maps:** loop over a slice with `range`, and fill a `map[string]string` with `m[key] = value`.
- **Interfaces:** a type satisfies an interface just by having the right methods. Nothing says "implements".
- **Errors are values:** a sentinel is a package-level `errors.New(...)`. Add context with `fmt.Errorf("...: %w", err)` and test for it later with `errors.Is`.

## The program

A `Student` has a name and a list of scores out of 100. A `Scale` turns an average into a letter-style label, and `Standard` is one implementation of it. `Report` ties it together: for every student, work out their average, label it, and record it in a map by name.

The two error cases are different, and that is the point of the exercise:

- A student with **no scores** has no average. `Average` returns `ErrNoScores`. That is not a failure of the whole report, so `Report` simply **skips** that student.
- A student with a score outside `0..100` is **bad data**. `Average` returns an error that wraps `ErrBadScore`. `Report` must stop and return `nil` and an error that names the student **and still wraps** `ErrBadScore`, so a caller can still ask `errors.Is(err, ErrBadScore)`.

So the same helper's error is handled in two different ways depending on what it means. In Go that is normal: decide at each level whether the error is yours to handle or to pass up.

## Labels

`Standard.Label` maps the average: 90 and above is `"A"`, 80 and above `"B"`, 70 and above `"C"`, and anything lower `"F"`. Check the thresholds from the top, and take care that exactly 90 is an `"A"`. `Report` takes a `Scale` interface, so a test can pass in a completely different scale and your code must use it rather than calling `Standard` directly.
