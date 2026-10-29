static RtcTime mk(int y, int mo, int d, int h, int mi, int s) {
    RtcTime t = {(uint16_t)y, (uint8_t)mo, (uint8_t)d, (uint8_t)h, (uint8_t)mi, (uint8_t)s};
    return t;
}

static int64_t secs_of_day(const RtcTime *t) { return t->hour * 3600 + t->minute * 60 + t->second; }
static int64_t diff_in_day(const RtcTime *a, const RtcTime *b) {
    int64_t d = secs_of_day(a) - secs_of_day(b);
    if (d < 0) d += 86400;
    if (d > 43200) d = 86400 - d;
    return d;
}

void kmain(void) {
    CHECK("bcd_to_bin converts 0x59 to 59", bcd_to_bin(0x59) == 59, "got %d", bcd_to_bin(0x59));
    CHECK("bcd_to_bin handles 0x00, 0x09, 0x10 and 0x99", bcd_to_bin(0x00) == 0 && bcd_to_bin(0x09) == 9 && bcd_to_bin(0x10) == 10 && bcd_to_bin(0x99) == 99,
          "got %d %d %d %d", bcd_to_bin(0x00), bcd_to_bin(0x09), bcd_to_bin(0x10), bcd_to_bin(0x99));

    bm_outb(0x70, 0x0A);
    uint8_t a_direct = bm_inb(0x71);
    CHECK("cmos_read(0x0A) matches the ports used directly", (cmos_read(0x0A) & 0x7F) == (a_direct & 0x7F), "got 0x%02x, direct read 0x%02x", cmos_read(0x0A), a_direct);
    bm_outb(0x70, 0x0B);
    uint8_t b_direct = bm_inb(0x71);
    CHECK("cmos_read(0x0B) reads status B", cmos_read(0x0B) == b_direct, "got 0x%02x, direct read 0x%02x", cmos_read(0x0B), b_direct);
    CHECK("rtc_update_in_progress returns exactly 0 or 1", rtc_update_in_progress() == 0 || rtc_update_in_progress() == 1, "returned %d", rtc_update_in_progress());

    RtcTime t = mk(0, 0, 0, 0, 0, 0);
    rtc_read(&t);
    CHECK("rtc_read returns a sane date", t.year >= 2024 && t.year <= 2200 && t.month >= 1 && t.month <= 12 && t.day >= 1 && t.day <= 31,
          "read %04d-%02d-%02d", t.year, t.month, t.day);
    CHECK("rtc_read returns a sane time of day", t.hour < 24 && t.minute < 60 && t.second < 60, "read %02d:%02d:%02d", t.hour, t.minute, t.second);
    int64_t u = rtc_to_unix(&t);
    CHECK("the clock is after 2024", u > 1704067200, "timestamp is %ld", (long)u);

    // The same moment must come out of the other number formats. Status B bit 2 = binary, bit 1 = 24-hour.
    uint8_t orig = b_direct;
    bm_outb(0x70, 0x0B); bm_outb(0x71, (uint8_t)(orig | 0x04));
    RtcTime tb = mk(0, 0, 0, 0, 0, 0);
    rtc_read(&tb);
    bm_outb(0x70, 0x0B); bm_outb(0x71, orig);
    CHECK("rtc_read understands binary mode (status B bit 2)", tb.year == t.year && tb.month == t.month && diff_in_day(&tb, &t) <= 5,
          "BCD read %04d-%02d-%02d %02d:%02d:%02d, binary read %04d-%02d-%02d %02d:%02d:%02d",
          t.year, t.month, t.day, t.hour, t.minute, t.second, tb.year, tb.month, tb.day, tb.hour, tb.minute, tb.second);

    bm_outb(0x70, 0x0B); bm_outb(0x71, (uint8_t)(orig & ~0x02));
    RtcTime th = mk(0, 0, 0, 0, 0, 0);
    rtc_read(&th);
    bm_outb(0x70, 0x0B); bm_outb(0x71, orig);
    CHECK("rtc_read understands 12-hour mode with the PM bit (status B bit 1)", th.year == t.year && diff_in_day(&th, &t) <= 5,
          "24-hour read %02d:%02d:%02d, 12-hour read %02d:%02d:%02d", t.hour, t.minute, t.second, th.hour, th.minute, th.second);

    RtcTime e0 = mk(1970, 1, 1, 0, 0, 0);
    RtcTime e1 = mk(1999, 12, 31, 23, 59, 59);
    RtcTime e2 = mk(2000, 1, 1, 0, 0, 0);
    CHECK("rtc_to_unix: 1970-01-01 is 0", rtc_to_unix(&e0) == 0, "got %ld", (long)rtc_to_unix(&e0));
    CHECK("rtc_to_unix: 1999-12-31 23:59:59 is 946684799", rtc_to_unix(&e1) == 946684799, "got %ld", (long)rtc_to_unix(&e1));
    CHECK("rtc_to_unix: 2000-01-01 is 946684800", rtc_to_unix(&e2) == 946684800, "got %ld", (long)rtc_to_unix(&e2));
    RtcTime l1 = mk(2024, 2, 29, 12, 34, 56);
    CHECK("rtc_to_unix: a leap day, 2024-02-29 12:34:56 is 1709210096", rtc_to_unix(&l1) == 1709210096, "got %ld", (long)rtc_to_unix(&l1));
    RtcTime l2 = mk(2000, 3, 1, 0, 0, 0);
    CHECK("rtc_to_unix: 2000 is a leap year (2000-03-01 is 951868800)", rtc_to_unix(&l2) == 951868800, "got %ld", (long)rtc_to_unix(&l2));
    RtcTime l3 = mk(2100, 3, 1, 0, 0, 0);
    CHECK("rtc_to_unix: 2100 is not a leap year (2100-03-01 is 4107542400)", rtc_to_unix(&l3) == 4107542400L, "got %ld", (long)rtc_to_unix(&l3));
    RtcTime l4 = mk(2038, 1, 19, 3, 14, 7);
    CHECK("rtc_to_unix: 2038-01-19 03:14:07 is 2147483647", rtc_to_unix(&l4) == 2147483647L, "got %ld", (long)rtc_to_unix(&l4));
    RtcTime m1 = mk(2023, 1, 31, 0, 0, 0), m2 = mk(2023, 3, 1, 0, 0, 0);
    CHECK("rtc_to_unix: January 31 to March 1 in a normal year is 29 days", rtc_to_unix(&m2) - rtc_to_unix(&m1) == 29 * 86400L, "difference is %ld seconds", (long)(rtc_to_unix(&m2) - rtc_to_unix(&m1)));
}
