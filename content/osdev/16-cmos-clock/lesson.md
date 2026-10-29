---
title: What Time Is It? The CMOS Clock
summary: Read the battery-backed real-time clock through the CMOS ports, wait out the update window, handle BCD and 12-hour modes, and convert to a Unix timestamp.
minutes: 40
difficulty: easy
tags: hardware, io, time
id: osdev/cmos-clock
mode: harness
---
The PIT you programmed earlier counts *ticks*, but it has no idea what day it is. Every PC also has a **real-time clock** (RTC) powered by a small battery. It keeps the calendar even when the machine is off, and a kernel reads it once at boot to learn the date.

## Two ports, many registers

The RTC lives inside a chip with 128 bytes of battery-backed memory, the **CMOS**. You reach it through just two ports:

- write the *register number* to port `0x70` (the **index** port),
- then read or write the value on port `0x71` (the **data** port).

```c
outb(0x70, reg);
uint8_t value = inb(0x71);
```

The registers you need:

| Reg | Meaning |
|---|---|
| `0x00` | seconds (0 to 59) |
| `0x02` | minutes |
| `0x04` | hours |
| `0x07` | day of month (1 to 31) |
| `0x08` | month (1 to 12) |
| `0x09` | year, **two digits** (`26` for 2026) |
| `0x0A` | status A: bit 7 = **update in progress** |
| `0x0B` | status B: bit 1 = 24-hour mode, bit 2 = **binary** (instead of BCD) |

Bit 7 of the index port also controls the non-maskable interrupt, so always send the register number with bit 7 clear (`reg & 0x7F`) unless you mean to change that.

## BCD

By default the clock stores each number as **binary-coded decimal**: one decimal digit per nibble. The time 12:34:56 is stored as `0x12`, `0x34`, `0x56`. To convert: `(v >> 4) * 10 + (v & 0x0F)`. Status B bit 2 says whether the clock is using plain binary instead; a correct driver checks it.

In 12-hour mode (status B bit 1 clear) the hour register's top bit means PM, and the hour counts 1 to 12. Convert: take away bit 7, then `12` becomes `0`, and add 12 if PM. Do the BCD conversion *after* removing the PM bit, otherwise `0x81` is read as a huge number.

## The update window

Once a second the clock increments its registers, and for a short moment they are inconsistent: if you read seconds just before 12:34:59 rolls over and minutes just after, you'd see 12:35:59, a minute in the future. Two defences are common:

1. Wait until bit 7 of status A is clear before reading.
2. Read all the registers, then read them again; if the two reads differ, start over.

You will do both.

## From calendar to Unix time

A **Unix timestamp** is the number of seconds since 1970-01-01 00:00:00 UTC. It's what most software wants, because subtracting two of them is easy. To compute it you need the number of days from 1970 to a given date. Do it without a loop over years using **days from civil**: shift the calendar so the year starts in March (then the leap day is the *last* day of the shifted year):

```c
int64_t y = year - (month <= 2);
int64_t era = y / 400;                       // y is positive here
int64_t yoe = y - era * 400;                  // 0..399
int64_t doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;   // 0..365
int64_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;                       // 0..146096
int64_t days = era * 146097 + doe - 719468;   // days since 1970-01-01
```

Then `seconds = days * 86400 + hour * 3600 + minute * 60 + second`. Leap years (divisible by 4, except centuries not divisible by 400) fall out of the arithmetic, so 2100 is correctly *not* a leap year.

## Debugging checklist

- **The minute is occasionally off by one:** you read during the update window. Wait for bit 7 of status A, and compare two reads.
- **Hours are wrong after noon in 12-hour mode:** strip the PM bit before the BCD conversion.
- **Timestamps are off by about a day around March:** the month offset in the days-from-civil formula is wrong.
- **The year is 1926:** the clock stores two digits, so add 2000.

## Your turn

The hidden tests check the pure conversions with known dates, and also read the real clock of the simulated machine in both BCD and binary mode.
