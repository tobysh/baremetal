`cmos_read` is two port operations: `outb(0x70, reg & 0x7F); return inb(0x71);`. `rtc_update_in_progress` is `cmos_read(0x0A) & 0x80`, turned into 0 or 1.
===
Write a helper `static void rtc_snapshot(RtcTime *t, uint8_t *raw_hour)` that reads the raw registers once. In `rtc_read`, loop: wait while `rtc_update_in_progress()`, take one snapshot, wait again, take a second, and repeat until the two match (`memcmp` or compare field by field).
===
Convert after the snapshot: read status B once (`cmos_read(0x0B)`). If bit 2 is clear, run every field through `bcd_to_bin`. For the hour: `pm = hour & 0x80; hour &= 0x7F;` then convert; if status B bit 1 is clear (12-hour mode) do `hour %= 12; if (pm) hour += 12;`.
===
In `rtc_to_unix`, use `int64_t` for everything. Days from civil: `y = year - (month <= 2)`, `era = y / 400`, `yoe = y - era*400`, `doy = (153*(month + (month > 2 ? -3 : 9)) + 2)/5 + day - 1`, `doe = yoe*365 + yoe/4 - yoe/100 + doy`, `days = era*146097 + doe - 719468`.
===
```c
uint8_t bcd_to_bin(uint8_t v) { return (uint8_t)((v >> 4) * 10 + (v & 0x0F)); }

int64_t rtc_to_unix(const RtcTime *t) {
    int64_t y = (int64_t)t->year - (t->month <= 2);
    int64_t era = y / 400;
    int64_t yoe = y - era * 400;
    int64_t doy = (153 * (t->month + (t->month > 2 ? -3 : 9)) + 2) / 5 + t->day - 1;
    int64_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    int64_t days = era * 146097 + doe - 719468;
    return days * 86400 + t->hour * 3600 + t->minute * 60 + t->second;
}
```
