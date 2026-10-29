The `outb` and `inb` port helpers and the `RtcTime` struct (`year` is the full year, for example 2026) are in the starter. Implement:

- `uint8_t bcd_to_bin(uint8_t v)` converts a BCD byte, so `0x59` becomes `59`.
- `uint8_t cmos_read(uint8_t reg)` selects `reg` (with bit 7 of the index cleared) on port `0x70` and returns the byte from port `0x71`.
- `int rtc_update_in_progress(void)` returns 1 if bit 7 of status register A (`0x0A`) is set.
- `void rtc_read(RtcTime *t)` fills `t` with the current date and time. It waits until no update is in progress, reads every field, and reads again until two consecutive reads agree. It honours status B: binary or BCD, 24-hour or 12-hour (with the PM bit). The year is `2000` plus the two-digit register.
- `int64_t rtc_to_unix(const RtcTime *t)` returns the Unix timestamp for `t` (UTC). It must be correct for every year from 1970 to 2399, including leap days and century years.
