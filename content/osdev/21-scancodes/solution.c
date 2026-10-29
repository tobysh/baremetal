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
    uint8_t st = inb(0x64);
    if (!(st & 1)) return -1;
    uint8_t b = inb(0x60);
    if (st & 0x20) return -1;
    return b;
}

int kbd_feed(KbdState *s, uint8_t sc) {
    if (sc == 0xE0) {
        s->e0 = 1;
        return 0;
    }
    int released = (sc & 0x80) != 0;
    uint8_t code = sc & 0x7F;
    if (s->e0) {
        s->e0 = 0;
        if (code == 0x1D) {
            s->ctrl = !released;
            return 0;
        }
        if (released) return 0;
        switch (code) {
        case 0x48: return KEY_UP;
        case 0x50: return KEY_DOWN;
        case 0x4B: return KEY_LEFT;
        case 0x4D: return KEY_RIGHT;
        default: return 0;
        }
    }
    if (code == 0x2A || code == 0x36) {
        uint8_t bit = code == 0x2A ? 1 : 2;
        if (released) s->shift &= (uint8_t)~bit;
        else s->shift |= bit;
        return 0;
    }
    if (code == 0x1D) {
        s->ctrl = !released;
        return 0;
    }
    if (code == 0x3A) {
        if (!released) s->caps ^= 1;
        return 0;
    }
    if (released || code >= 58) return 0;
    char base = scan_normal[code];
    int is_letter = base >= 'a' && base <= 'z';
    if (s->ctrl && is_letter) return base - 'a' + 1;
    int upper = is_letter ? (((s->shift != 0) ^ s->caps) != 0) : (s->shift != 0);
    return (unsigned char)(upper ? scan_shifted[code] : base);
}

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
