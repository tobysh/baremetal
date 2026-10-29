#include <stdint.h>
#include <stddef.h>

void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);

typedef struct {
    uint16_t year;   // full year, e.g. 2026
    uint8_t month;   // 1 to 12
    uint8_t day;     // 1 to 31
    uint8_t hour;    // 0 to 23
    uint8_t minute;  // 0 to 59
    uint8_t second;  // 0 to 59
} RtcTime;

uint8_t bcd_to_bin(uint8_t v) {
    return (uint8_t)((v >> 4) * 10 + (v & 0x0F));
}

uint8_t cmos_read(uint8_t reg) {
    outb(0x70, reg & 0x7F);
    return inb(0x71);
}

int rtc_update_in_progress(void) {
    return (cmos_read(0x0A) & 0x80) != 0;
}

typedef struct { uint8_t sec, min, hour, day, month, year; } Raw;

static void snapshot(Raw *r) {
    while (rtc_update_in_progress()) {
    }
    r->sec = cmos_read(0x00);
    r->min = cmos_read(0x02);
    r->hour = cmos_read(0x04);
    r->day = cmos_read(0x07);
    r->month = cmos_read(0x08);
    r->year = cmos_read(0x09);
}

static int same(const Raw *a, const Raw *b) {
    return a->sec == b->sec && a->min == b->min && a->hour == b->hour &&
           a->day == b->day && a->month == b->month && a->year == b->year;
}

void rtc_read(RtcTime *t) {
    Raw a, b;
    snapshot(&a);
    do {
        b = a;
        snapshot(&a);
    } while (!same(&a, &b));
    uint8_t status_b = cmos_read(0x0B);
    int pm = a.hour & 0x80;
    a.hour &= 0x7F;
    if (!(status_b & 0x04)) {
        a.sec = bcd_to_bin(a.sec);
        a.min = bcd_to_bin(a.min);
        a.hour = bcd_to_bin(a.hour);
        a.day = bcd_to_bin(a.day);
        a.month = bcd_to_bin(a.month);
        a.year = bcd_to_bin(a.year);
    }
    if (!(status_b & 0x02)) {
        a.hour %= 12;
        if (pm) a.hour += 12;
    }
    t->year = (uint16_t)(2000 + a.year);
    t->month = a.month;
    t->day = a.day;
    t->hour = a.hour;
    t->minute = a.min;
    t->second = a.sec;
}

int64_t rtc_to_unix(const RtcTime *t) {
    int64_t y = (int64_t)t->year - (t->month <= 2);
    int64_t era = y / 400;
    int64_t yoe = y - era * 400;
    int64_t doy = (153 * (t->month + (t->month > 2 ? -3 : 9)) + 2) / 5 + t->day - 1;
    int64_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    int64_t days = era * 146097 + doe - 719468;
    return days * 86400 + t->hour * 3600 + t->minute * 60 + t->second;
}

static void put(char c) {
    while ((inb(0x3FD) & 0x20) == 0) {
    }
    outb(0x3F8, (uint8_t)c);
}
static void put2(unsigned v) {
    put((char)('0' + v / 10));
    put((char)('0' + v % 10));
}
void kmain(void) {
    RtcTime t;
    rtc_read(&t);
    put2(t.hour); put(':'); put2(t.minute); put(':'); put2(t.second); put('\r'); put('\n');
}
