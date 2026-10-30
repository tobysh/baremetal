#include <stdio.h>
#include <stdint.h>
int main(void){
  unsigned x;
  while (scanf("%u", &x) == 1) {
    unsigned r = 0;
    for (int i = 0; i < 8; i++) r |= ((x >> i) & 1u) << (7 - i);
    printf("%u\n", r);
  }
  return 0;
}
