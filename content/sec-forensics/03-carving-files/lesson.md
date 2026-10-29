---
title: Carve a synthetic image
summary: Locate complete JPEG-like and PNG-like byte ranges while respecting input and output bounds.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-forensics/carving-files
mode: harness
---
Carving searches raw bytes for recognizable boundaries when a directory entry is unavailable. In this simplified image, a JPEG-like object starts `ff d8 ff` and ends at the first following `ff d9`. A PNG-like object starts with the full PNG signature and ends eight bytes after the first following ASCII `IEND` marker: four letters plus four CRC bytes.

Scan from low offsets to high offsets. When a complete object is found, report its type, start offset and length, then continue after it so the reported ranges do not overlap. When a header has no complete footer, advance one byte and continue searching. Stop when the output array is full. Never subtract or read past an unvalidated end offset.

These are deliberately synthetic formats. A real PNG parser follows chunk lengths and validates structure/CRCs; a real JPEG parser understands marker escaping and segments. Coincidental markers, fragmented files and embedded objects can confuse a signature-only carver. A recovered range is a candidate artifact, not proof of a usable original file. Keep the original image untouched and record offsets so an analyst can reproduce each finding.
