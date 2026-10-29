#include <stdint.h>
#include <stddef.h>

uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t value);

#define KEY_UP    0x101
#define KEY_DOWN  0x102
#define KEY_LEFT  0x103
#define KEY_RIGHT 0x104

typedef struct {
    uint8_t shift;   // bit 0: left shift is down, bit 1: right shift is down
    uint8_t ctrl;    // 1 while a Ctrl key is down
    uint8_t caps;    // 1 while caps lock is on
    uint8_t e0;      // 1 if the previous byte was the 0xE0 prefix
} KbdState;

// Scancode set 1, make codes 0x00 to 0x39. 0 means "no character".
static const char scan_normal[59] =
    "\0\033" "1234567890-=\b\t" "qwertyuiop[]\n" "\0" "asdfghjkl;'`" "\0" "\\zxcvbnm,./" "\0" "*" "\0" " ";
static const char scan_shifted[59] =
    "\0\033" "!@#$%^&*()_+\b\t" "QWERTYUIOP{}\n" "\0" "ASDFGHJKL:\"~" "\0" "|ZXCVBNM<>?" "\0" "*" "\0" " ";

int kbd_poll(void) {
    return -1;
}

int kbd_feed(KbdState *s, uint8_t sc) {
    return 0;
}

// Optional: press Run to echo what you type... there is no keyboard in the test machine,
// so this just decodes one fixed sequence and prints it on the serial port.
void kmain(void) {
    KbdState st = {0, 0, 0, 0};
    const uint8_t seq[] = {0x2A, 0x23, 0xA3, 0xAA, 0x17, 0x1C};   // H i Enter
    for (size_t i = 0; i < sizeof seq; i++) {
        int c = kbd_feed(&st, seq[i]);
        if (c > 0 && c < 256) {
            while ((inb(0x3FD) & 0x20) == 0) {
            }
            outb(0x3F8, (uint8_t)c);
        }
    }
}
