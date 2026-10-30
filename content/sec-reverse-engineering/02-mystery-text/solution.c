#include <stdio.h>
#include <string.h>
int main(void){
  char line[512];
  while (fgets(line, sizeof line, stdin)) {
    size_t n = strcspn(line, "\r\n"); line[n] = 0;
    for (size_t i = 0; i < n;) {
      size_t j = i; while (j < n && line[j] == line[i]) j++;
      size_t run = j - i;
      if (run >= 3) printf("%c%zu", line[i], run);
      else for (size_t k = 0; k < run; k++) putchar(line[i]);
      i = j;
    }
    putchar('\n');
  }
  return 0;
}
