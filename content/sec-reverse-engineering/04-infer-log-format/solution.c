#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
static uint8_t F[70000]; static size_t FL;
static void rd_hex(void) {
  int hi = -1, c; FL = 0;
  while ((c = getchar()) != EOF) {
    int v = (c >= '0' && c <= '9') ? c - '0' : (c >= 'a' && c <= 'f') ? c - 'a' + 10 : (c >= 'A' && c <= 'F') ? c - 'A' + 10 : -1;
    if (v < 0) continue;
    if (hi < 0) hi = v; else { if (FL < sizeof F) F[FL++] = (uint8_t)(hi * 16 + v); hi = -1; }
  }
}
int main(void){
  rd_hex();
  if (FL < 5) { puts("error: truncated header"); return 0; }
  if (F[0] != 'R' || F[1] != 'L') { puts("error: bad magic"); return 0; }
  if (F[2] != 1) { printf("error: unsupported version %u\n", F[2]); return 0; }
  unsigned count = F[3] | (F[4] << 8); size_t pos = 5;
  static const char *LV[] = {"DEBUG","INFO","WARN","ERROR"};
  for (unsigned i = 1; i <= count; i++) {
    if (pos + 6 > FL) { printf("error: truncated record %u\n", i); return 0; }
    unsigned lv = F[pos]; uint32_t ts = F[pos+1] | (F[pos+2] << 8) | (F[pos+3] << 16) | ((uint32_t)F[pos+4] << 24);
    unsigned len = F[pos+5];
    if (pos + 6 + len > FL) { printf("error: truncated record %u\n", i); return 0; }
    if (lv < 4) printf("%u %s ", ts, LV[lv]); else printf("%u L%u ", ts, lv);
    for (unsigned k = 0; k < len; k++) { uint8_t c = F[pos+6+k]; putchar(c >= 32 && c < 127 ? c : '.'); }
    putchar('\n'); pos += 6 + len;
  }
  if (pos < FL) printf("warning: %zu trailing bytes\n", FL - pos);
  printf("records: %u\n", count);
  return 0;
}
