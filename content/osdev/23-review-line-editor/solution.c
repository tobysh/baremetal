#include <stdint.h>
#include <stddef.h>

uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t value);

#define COM1 0x3F8

#define KEY_UP    0x101
#define KEY_DOWN  0x102
#define KEY_LEFT  0x103
#define KEY_RIGHT 0x104

#define LINE_MAX 16   // 15 characters and the terminating NUL

typedef struct {
    uint8_t shift;   // bit 0: left shift is down, bit 1: right shift is down
    uint8_t ctrl;
    uint8_t caps;
    uint8_t e0;
} KbdState;

typedef struct {
    char buf[LINE_MAX];
    size_t len;
    int done;        // 1 after Enter, until the line is taken or a new character arrives
} LineEditor;

// ---- Provided: the serial driver and the scancode decoder ----
void serial_init(void) {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 3);
    outb(COM1 + 1, 0);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x03);
}

void serial_putc(char c) {
    while ((inb(COM1 + 5) & 0x20) == 0) {
    }
    outb(COM1, (uint8_t)c);
}

// Scancode set 1, make codes 0x00 to 0x39. 0 means "no character".
static const char scan_normal[59] =
    "\0\033" "1234567890-=\b\t" "qwertyuiop[]\n" "\0" "asdfghjkl;'`" "\0" "\\zxcvbnm,./" "\0" "*" "\0" " ";
static const char scan_shifted[59] =
    "\0\033" "!@#$%^&*()_+\b\t" "QWERTYUIOP{}\n" "\0" "ASDFGHJKL:\"~" "\0" "|ZXCVBNM<>?" "\0" "*" "\0" " ";

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


// ---- Your code ----

static void erase_one(void) {
    serial_putc('\b');
    serial_putc(' ');
    serial_putc('\b');
}

void line_init(LineEditor *e) {
    e->len = 0;
    e->done = 0;
    e->buf[0] = 0;
}

int line_feed_char(LineEditor *e, int ch) {
    if (e->done) line_init(e);
    int finished = 0;
    if (ch == '\n') {
        serial_putc('\r');
        serial_putc('\n');
        e->done = 1;
        finished = 1;
    } else if (ch == '\b' || ch == 127) {
        if (e->len > 0) {
            e->len--;
            erase_one();
        }
    } else if (ch == 21) {
        while (e->len > 0) {
            e->len--;
            erase_one();
        }
    } else if (ch == 3) {
        e->len = 0;
        serial_putc('^');
        serial_putc('C');
        serial_putc('\r');
        serial_putc('\n');
    } else if (ch >= 32 && ch <= 126) {
        if (e->len < LINE_MAX - 1) {
            e->buf[e->len++] = (char)ch;
            serial_putc((char)ch);
        }
    }
    e->buf[e->len] = 0;
    return finished;
}

int line_feed_scancode(LineEditor *e, KbdState *k, uint8_t sc) {
    int c = kbd_feed(k, sc);
    if (c <= 0 || c > 255) return 0;
    return line_feed_char(e, c);
}

const char *line_text(LineEditor *e) {
    return e->buf;
}

size_t line_take(LineEditor *e, char *out, size_t cap) {
    size_t n = e->len;
    if (cap > 0) {
        size_t m = n < cap - 1 ? n : cap - 1;
        for (size_t i = 0; i < m; i++) out[i] = e->buf[i];
        out[m] = 0;
    }
    line_init(e);
    return n;
}

void kmain(void) {
    serial_init();
    LineEditor e;
    KbdState k = {0, 0, 0, 0};
    line_init(&e);
    const uint8_t keys[] = {0x26, 0x1F, 0x1C};
    for (size_t i = 0; i < sizeof keys; i++) line_feed_scancode(&e, &k, keys[i]);
}
