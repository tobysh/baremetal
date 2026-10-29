---
title: "Review: Record Toolkit"
summary: A consolidation exercise. Combine struct fields, bounded string copying, arrays of records and in-place updates in four small tools. No new ideas.
minutes: 20
difficulty: medium
kind: review
tags: structs, strings, arrays
id: c-data/review-records
mode: harness
---
This is a **review lesson**. It introduces nothing new. Each function combines familiar pieces with a small twist.

## What you already know

- **[Structs That Hold Text](/learn/c-data/structs-with-text):** a struct can carry a `char` array, and copying text in needs a bound and a terminator.
- **[Arrays of Structs](/learn/c-data/arrays-of-structs):** loop over `people[i].field`, and update records in place through the array.
- **[Strings Are Arrays of Characters](/learn/c-data/char-arrays):** walk until `'\0'`, and turn lowercase letters into capitals with arithmetic on characters.
- **[Searching and Checking Arrays](/learn/c-data/searching):** return the index of the best match, early.

## The twists

1. **Build a record from text.** `person_make` copies a name (at most 19 characters) like `item_make` did.
2. **Compare only the start of two strings.** `name_starts_with` checks the first characters of a name against a prefix. It may return as soon as one pair differs.
3. **Update text inside records.** `capitalise_names` makes the first letter of every person's name uppercase.
4. **Find the extreme of a field.** `oldest_index` is a maximum search that returns an index.
