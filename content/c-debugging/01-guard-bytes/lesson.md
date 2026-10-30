---
title: Guard Bytes
summary: Neighbouring bytes change after a harmless-looking call. Read the memory dump, name the faulty write, and truncate long input instead of spilling over.
minutes: 20
difficulty: easy
tags: debugging, memory-corruption, evidence
id: c-debugging/guard-bytes
mode: harness
---
When a program misbehaves, resist the urge to edit. First collect **evidence**, then decide what it proves.

## Guard bytes

A *guard* (or canary) is a few bytes with a known pattern placed on each side of a buffer. Nothing should ever write to them. If a guard changes, something wrote outside its buffer, and the guard tells you which side.

```c
typedef struct {
    unsigned char guard_lo[4];   // 0xAA 0xAA 0xAA 0xAA
    char          name[8];
    unsigned char guard_hi[4];   // 0xAA 0xAA 0xAA 0xAA
} record;
```

Fields of a struct sit next to each other in memory, so `name[8]` is directly followed by `guard_hi`.

## The evidence

A bug report says: "after `set_name(&r, "Bartholomew")` the record is damaged". Here is a dump of the record before and after, 16 bytes in memory order:

```text
before: AA AA AA AA 00 00 00 00 00 00 00 00 AA AA AA AA
after : AA AA AA AA 42 61 72 74 68 6F 6C 6F 6D 65 77 00
        guard_lo   name (8 bytes)           guard_hi
```

Read it carefully:

- `guard_lo` is untouched, so nothing wrote *before* the name.
- The name bytes spell `Bartholo`, which is the first 8 letters of the input. The copy started in the right place.
- The next bytes are `6D 65 77` (`mew`), the rest of the input, followed by `00`, the string terminator. They sit where `guard_hi` was.

That proves the copy **kept going past the end of `name`** and wrote the remaining letters and the terminator over the upper guard. It does not prove anything about other callers; check those separately.

Notice what is *not* in the evidence: no crash. Overflows inside a struct often stay silent, which is why guards and sanitizers exist.

## What a correct fix preserves

Copying fewer bytes is not enough. The result must still be a valid C string, so the last byte of `name` has to be `'\0'`. A caller also wants to know whether input was lost, so `set_name` returns **how many characters were dropped**.

## Your turn

Fix `set_name`. Changing the struct or the guards is not the fix: the guards are the instrument that found the bug.
