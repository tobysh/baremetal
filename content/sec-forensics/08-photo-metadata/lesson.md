---
title: Read synthetic TIFF metadata
summary: Decode a bounded TIFF-style directory in either byte order and normalize fictional timestamps to UTC.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-forensics/photo-metadata
mode: harness
---
## The forensic question

An image claims it was taken with a certain camera model at a certain time. Where does that claim come from, and how far can you trust it? Cameras store such facts as **metadata** inside the file, in a TIFF-style directory. An editor can rewrite any of it, and a camera clock can simply be wrong, so metadata is a lead, never a proof. This lesson reads a small synthetic subset from standalone data.

## The format

```text
offset  size  meaning
0       2     byte order: "II" little-endian or "MM" big-endian
2       2     the number 42 (in that byte order)
4       4     offset of the directory
dir     2     entry count N
dir+2   12*N  entries: tag(2) type(2) count(4) value-or-offset(4)
```

Tags used here: `0x010f` Make, `0x0110` Model, `0x0100` width, `0x0101` height, `0x0132` timestamp. Types: 2 = ASCII (count includes the terminating zero), 3 = SHORT (16-bit), 4 = LONG (32-bit). An ASCII value of 4 bytes or fewer lives inside the entry; longer ones live at the offset.

A little-endian sample with one inline string (hex, bytes shown in file order):

```text
49 49 2a 00 08 00 00 00        "II", 42, directory at 8
02 00                          2 entries
0f 01 02 00 04 00 00 00 43 61 6d 00    Make: ASCII, count 4, inline "Cam\0"
00 01 03 00 01 00 00 00 80 02 00 00    width: SHORT, count 1, 0x0280 = 640
```

## Worked example

1. `49 49` is "II": every multi-byte number is little-endian. `2a 00` is 42, so this is TIFF-like data. The directory offset `08 00 00 00` is 8.
2. At offset 8 the count is `02 00` = 2, so 24 bytes of entries follow: check that they fit in `n`.
3. Entry 0: tag `0f 01` = 0x010f (Make), type 2, count 4. Four bytes fit inline, and the last is zero, so the string is `Cam`.
4. Entry 1: tag 0x0100 (width), type 3 (SHORT), count 1. For a SHORT the value is the first two bytes of the value field: `80 02` = 0x0280 = 640.

For a timestamp such as `2026:03:02 09:15:00`, convert to Unix seconds assuming UTC: count the days from 1970-01-01 (365 or 366 per year, month lengths, leap years), then add hours, minutes and seconds.

## Pitfalls

- **Byte order is per file.** Use one helper that reads 2 or 4 bytes for either order; do not cast pointers.
- **Validate before reading.** Check the directory offset, then `count <= (n - off) / 12`, then every string offset with `v <= n && c <= n - v`. Never write `v + c <= n`, which can wrap.
- **Terminator.** A string must end in a zero inside its own count, and must fit your field, terminator included.
- **Leap years.** 2024 and 2000 are leap, 2100 and 1900 are not (divisible by 100 but not 400).
- **No time zone.** Real EXIF timestamps usually carry none; here UTC is assumed explicitly.

## What a defender concludes

Report "metadata says Examplecam EC-100 at 2026:03:02 09:15:00, assuming UTC", not "taken at". Compare it with file system timestamps, other images from the same device and the upload time. A mismatch is a finding; agreement is only consistent, not proven.
