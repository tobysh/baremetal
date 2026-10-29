---
title: Parse length-prefixed records
summary: Walk a type-length-value buffer without trusting the lengths it announces, and tell truncation apart from a missing record.
minutes: 30
difficulty: medium
tags: security, parsing, input-validation
id: sec-c-bugs/tlv-parser
mode: harness
---
Many binary formats store records as type, length, value (TLV). The length comes from the sender, so it is attacker-controlled data. A parser that adds the announced length to its position and trusts the result will read past the end of the buffer, or skip into the middle of the next record. Both mistakes have caused real information leaks, and the defence is the same in each case: before using a length, compare it with the number of bytes that really remain.

Here a record is one type byte, a two-byte big-endian length and then that many value bytes. `tlv_find` scans from the start and reports the first record of the wanted type. Check the header fits (`len - pos >= 3`) and the value fits (`l <= len - pos`) by subtraction on values already known to be in range; writing `pos + l <= len` can overflow for large lengths on other record formats.

Notice the three outcomes. Finding the record means success even if junk follows it, because the parser never looks that far. A malformed record met before the wanted type is an error, which is different from a clean end with no match. Keeping those results distinct lets the caller log an attack differently from an ordinary miss. All buffers here are synthetic.
