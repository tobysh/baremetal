#include <stdio.h>
#include <stdint.h>
#include <string.h>
static uint8_t chk(const uint8_t *b, size_t n){ uint8_t x = 0x5A; for (size_t i = 0; i < n; i++) x ^= b[i]; return x; }
static void emit(uint8_t op, uint8_t seq, const uint8_t *pl, size_t n){
  uint8_t o[300]; o[0] = 0xB7; o[1] = op; o[2] = seq; o[3] = (uint8_t)n; memcpy(o + 4, pl, n); o[4 + n] = chk(o, 4 + n);
  for (size_t i = 0; i < 5 + n; i++) printf("%s%02x", i ? " " : "", o[i]);
  putchar('\n');
}
int main(void){
  char line[1200];
  while (fgets(line, sizeof line, stdin)) {
    uint8_t b[300]; size_t n = 0; int hi = -1;
    for (char *p = line; *p; p++) {
      int c = *p, v = (c>='0'&&c<='9')?c-'0':(c>='a'&&c<='f')?c-'a'+10:(c>='A'&&c<='F')?c-'A'+10:-1;
      if (v < 0) continue;
      if (hi < 0) hi = v; else { if (n < sizeof b) b[n++] = (uint8_t)(hi*16+v); hi = -1; }
    }
    if (n < 5 || b[0] != 0xB7) { puts("drop"); continue; }
    uint8_t seq = b[2];
    if (n != 5u + b[3]) { uint8_t e = 3; emit(0xFF, seq, &e, 1); continue; }
    if (chk(b, n - 1) != b[n-1]) { uint8_t e = 2; emit(0xFF, seq, &e, 1); continue; }
    uint8_t len = b[3]; const uint8_t *pl = b + 4; uint8_t out[300];
    switch (b[1]) {
      case 1: emit(0x81, seq, pl, len); break;
      case 2: for (int i = 0; i < len; i++) out[i] = pl[len-1-i]; emit(0x82, seq, out, len); break;
      case 3: { unsigned s = 0; for (int i = 0; i < len; i++) s += pl[i]; out[0] = s >> 8; out[1] = s & 255; emit(0x83, seq, out, 2); break; }
      case 4: for (int i = 0; i < len; i++) out[i] = (pl[i] >= 'a' && pl[i] <= 'z') ? pl[i] - 32 : pl[i]; emit(0x84, seq, out, len); break;
      default: { uint8_t e = 1; emit(0xFF, seq, &e, 1); }
    }
  }
  return 0;
}
