---
title: Read synthetic TIFF metadata
summary: Decode a bounded TIFF-style directory in either byte order and normalize fictional timestamps to UTC.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-forensics/photo-metadata
mode: harness
---
Photo metadata is structured evidence, not a trustworthy account of when or where an image was created. Editors can alter tags and camera clocks can be wrong. This lesson reads standalone synthetic TIFF-style data, not an entire JPEG/EXIF container.

The header is II for little-endian or MM for big-endian, a 16-bit value 42, and a 32-bit offset to a directory. At that offset is a 16-bit entry count followed by 12-byte entries: tag (16 bits), type (16), count (32) and value/offset (32). Validate the directory range before traversing it. Supported tags are Make 0x010f, Model 0x0110, width 0x0100, height 0x0101 and timestamp 0x0132. Numeric values are one SHORT (type 3) or LONG (4); ASCII is type 2 with a terminating zero included in count. Up to four ASCII bytes are inline; longer strings use an offset. Unknown tags/types are ignored.

Validate each accessed range using subtraction after checking the offset, and cap copies into the result fields. Missing tags remain empty/zero. Timestamps use `YYYY:MM:DD HH:MM:SS`. For this fictional dataset we explicitly assume UTC; real EXIF timestamps often omit a zone, so do not silently apply that assumption to real evidence. Validate Gregorian dates before converting days since 1970 to seconds.
