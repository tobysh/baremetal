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
    return 0;
}

uint8_t cmos_read(uint8_t reg) {
    return 0;
}

int rtc_update_in_progress(void) {
    return 0;
}

void rtc_read(RtcTime *t) {
}

int64_t rtc_to_unix(const RtcTime *t) {
    return 0;
}

// Optional: press Run to see the clock on the serial port.
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
