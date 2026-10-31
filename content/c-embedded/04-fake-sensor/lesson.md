---
title: A Fake Temperature Sensor
summary: Start a conversion, wait with a deadline, handle a fault flag, then decode a 12-bit two's complement reading into millidegrees.
minutes: 30
difficulty: medium
tags: embedded, sensors, two-complement
id: c-embedded/fake-sensor
mode: harness
---
This lesson combines the first three: field extraction, W1C acknowledgement and a bounded wait, in one small driver.

## The sensor

| offset | name | meaning |
|---|---|---|
| 0x00 | CTRL | write bit 0 = START a conversion |
| 0x04 | STATUS | bit 0 DONE (read-only), bit 1 FAULT (write-one-to-clear) |
| 0x08 | DATA | bits 11:0 reading; **reading DATA consumes the result and clears DONE**. Upper bits are junk. |

After START, the conversion takes a device-dependent number of ticks and then sets DONE, or sets FAULT instead (a failed conversion never sets DONE). While FAULT is set the device ignores START, so a driver that does not clear it is stuck for ever.

The reading is a 12-bit **two's complement** number in units of 1/16 degree Celsius. `0x190` is 400/16 = 25.0 C, and `0xE70` is -400/16 = -25.0 C. To sign-extend 12 bits: `(int)((raw & 0xFFF) ^ 0x800) - 0x800`.

Convert to millidegrees as `raw * 62.5`, **rounded to the nearest integer with ties away from zero** (for example raw 1 gives 63 and raw -1 gives -63). Integer arithmetic only.

## Driver contract

`int sensor_read_mC(bus_t *b, int32_t *out, uint32_t timeout)`:

1. Write START to CTRL once.
2. Poll STATUS until DONE or FAULT, giving up when more than `timeout` ticks have elapsed since entry (look first, then check the clock, as in the previous lesson).
3. FAULT: acknowledge it by writing exactly the FAULT bit to STATUS, and return -2.
4. Timeout: return -1.
5. DONE: read DATA exactly once, convert, store in `*out`, return 0.

`*out` is left untouched on any error, and DATA is not read on an error. In the simulator each STATUS read costs one tick.

**Limitation:** a real sensor's conversion time varies with temperature and supply, and a fault may be intermittent. The simulator uses a fixed latency per test and a scheduled fault.
