// The tests read the real VGA text memory and the real CRT cursor registers.
static volatile uint16_t *const screen = (volatile uint16_t *)0xB8000;

static unsigned cursor(void) {
    bm_outb(0x3D4, 0x0F);
    unsigned lo = bm_inb(0x3D5);
    bm_outb(0x3D4, 0x0E);
    unsigned hi = bm_inb(0x3D5);
    return (hi << 8) | lo;
}

static void puts_(const char *s) { for (; *s; s++) term_putc(*s); }
static unsigned cell(int r, int c) { return screen[r * 80 + c]; }

void kmain(void) {
    CHECK("vga_entry puts the character in the low byte and colour in the high byte",
          vga_entry('A', 0x0F) == 0x0F41 && vga_entry(' ', 0x1E) == 0x1E20, "got 0x%04x and 0x%04x", vga_entry('A', 0x0F), vga_entry(' ', 0x1E));
    CHECK("vga_entry does not smear a negative char into the colour",
          vga_entry((char)0xE9, 0x07) == 0x07E9, "got 0x%04x", vga_entry((char)0xE9, 0x07));

    for (int i = 0; i < 2000; i++) screen[i] = 0xFFFF;
    term_init();
    int blank = 1; unsigned bad = 0;
    for (int i = 0; i < 2000; i++) if (screen[i] != 0x0720) { blank = 0; bad = screen[i]; break; }
    CHECK("term_init clears the whole screen to spaces", blank, "a cell still holds 0x%04x", bad);
    CHECK("term_init moves the hardware cursor to the top-left", cursor() == 0, "cursor is at %u", cursor());

    term_putc('H'); term_putc('i');
    CHECK("term_putc writes characters left to right", cell(0, 0) == 0x0748 && cell(0, 1) == 0x0769, "cells are 0x%04x 0x%04x", cell(0, 0), cell(0, 1));
    CHECK("term_putc moves the hardware cursor", cursor() == 2, "cursor is at %u", cursor());

    term_init(); puts_("ab\ncd");
    CHECK("\\n starts the next row at column 0", cell(0, 0) == 0x0761 && cell(0, 1) == 0x0762 && cell(1, 0) == 0x0763 && cell(1, 1) == 0x0764, "row 1 starts with 0x%04x", cell(1, 0));
    CHECK("the cursor follows a newline", cursor() == 82, "cursor is at %u", cursor());

    term_init(); puts_("abc\rX");
    CHECK("\\r returns to column 0 of the same row", cell(0, 0) == 0x0758 && cell(0, 1) == 0x0762 && cursor() == 1, "cell 0 is 0x%04x, cursor %u", cell(0, 0), cursor());

    term_init(); puts_("ab\b");
    CHECK("\\b steps back and blanks the cell", cell(0, 1) == 0x0720 && cell(0, 0) == 0x0761 && cursor() == 1, "cell 1 is 0x%04x, cursor %u", cell(0, 1), cursor());
    term_init(); term_putc('\b');
    CHECK("\\b at column 0 does nothing", cursor() == 0, "cursor is at %u", cursor());

    term_init(); term_putc('\t');
    CHECK("\\t moves to column 8", cursor() == 8, "cursor is at %u", cursor());
    term_putc('\t');
    CHECK("\\t from a tab stop moves a whole 8 columns", cursor() == 16, "cursor is at %u", cursor());

    term_init();
    for (int i = 0; i < 80; i++) term_putc('x');
    CHECK("text wraps to the next row after column 79", cell(0, 79) == 0x0778 && cursor() == 80, "cursor is at %u", cursor());
    term_putc('y');
    CHECK("the first character after a wrap lands on row 1", cell(1, 0) == 0x0779, "cell is 0x%04x", cell(1, 0));

    term_set_color(14, 1);
    term_init(); term_putc('X');
    CHECK("term_set_color uses fg in the low nibble and bg in the high nibble", cell(0, 0) == 0x1E58, "cell is 0x%04x", cell(0, 0));
    CHECK("term_init clears with the current colour", cell(5, 5) == 0x1E20, "cell is 0x%04x", cell(5, 5));
    term_set_color(7, 0);

    term_init();
    for (int i = 0; i < 25; i++) { term_putc((char)('A' + i)); term_putc('\n'); }
    CHECK("scrolling moves every row up by one", cell(0, 0) == 0x0742 && cell(12, 0) == 0x074E && cell(23, 0) == 0x0759,
          "rows 0, 12, 23 start with 0x%04x 0x%04x 0x%04x", cell(0, 0), cell(12, 0), cell(23, 0));
    int clear = 1;
    for (int c = 0; c < 80; c++) if (cell(24, c) != 0x0720) clear = 0;
    CHECK("scrolling clears the bottom row", clear, "the last row still has text");
    CHECK("after scrolling the cursor is at the start of the last row", cursor() == 24 * 80, "cursor is at %u", cursor());

    term_set_color(14, 1);
    term_putc('Q'); term_putc('\n');
    CHECK("the row cleared by a scroll uses the current colour", cell(24, 70) == 0x1E20, "cell is 0x%04x", cell(24, 70));
    term_set_color(7, 0);

    term_init();
    for (int i = 0; i < 80 * 25; i++) term_putc('z');
    CHECK("filling the last cell scrolls once and leaves the cursor on the last row", cursor() == 24 * 80 && cell(0, 0) == 0x077A, "cursor is at %u", cursor());
}
