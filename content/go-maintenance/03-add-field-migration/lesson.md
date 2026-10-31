---
title: Add a Field, Migrate the Data
summary: Add a phone number to a stored member format while old and new records coexist in one file, without losing comments or breaking old data.
minutes: 40
difficulty: hard
tags: migration, formats, compatibility
id: go-maintenance/add-field-migration
mode: harness
---
Code changes ship in minutes; stored data lives for years. When a record gains a field, files written yesterday still exist, and some may be read by the new code tomorrow. A safe change follows these rules:

- **Readers accept every version that was ever written.** Old records get a sensible default for the new field.
- **Versions are recognisable.** Give the new format a marker that the old format could never have produced, so the reader never has to guess.
- **Writers emit one canonical version.** After a load and save, the file is on the new format.
- **A migration is a pure transformation.** It must be idempotent (running it twice changes nothing more), and it must fail without partial output when it meets bad data.
- **Do not lose what you do not understand.** Comments and blank lines belong to the humans who edit the file.

## The formats

Version 1 (what is on disk today): `id|name|email`, fields trimmed. It cannot hold a `|` or a newline, so `Add` refuses them.

Version 2 (new): `v2|id|name|email|phone`. Because ids are numbers, a line starting `v2|` can never be a v1 record. Fields are escaped: `\\` for a backslash, `\|` for a pipe, `\n` and `\r` for line breaks. v2 fields are **not** trimmed.

## Honest limits

The tests check your parser against hand-written lines and check round trips with awkward text. They cannot check that you have thought about rollback: once v2 is written, an old binary cannot read the file. In real life that is why the read side ships first, and the write side later.
