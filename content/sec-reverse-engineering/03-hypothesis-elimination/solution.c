#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int digit_sum(int x){ long v = x < 0 ? -(long)x : x; int s = 0; while (v) { s += (int)(v % 10); v /= 10; } return s; }
typedef struct { const char *name; int (*fn)(int); } H;
static int h_double(int x){return 2*x;} static int h_square(int x){return x*x;}
static int h_plus_ten(int x){return x+10;} static int h_abs(int x){return x<0?-x:x;}
static int h_negate(int x){return -x;} static int h_is_odd(int x){return x&1;}
static int h_sign(int x){return (x>0)-(x<0);} static int h_clamp_100(int x){return x>100?100:x<-100?-100:x;}
static int h_mod_7(int x){return x%7;}
static const H TAB[] = {{"double",h_double},{"square",h_square},{"plus_ten",h_plus_ten},{"abs",h_abs},{"negate",h_negate},{"is_odd",h_is_odd},{"sign",h_sign},{"clamp_100",h_clamp_100},{"mod_7",h_mod_7},{"digit_sum",digit_sum}};
int main(void){
  int n; static int in[1000], out[1000];
  if (scanf("%d", &n) != 1) return 0;
  for (int i = 0; i < n; i++) scanf("%d %d", &in[i], &out[i]);
  int m; if (scanf("%d", &m) != 1) return 0;
  char names[64][32]; int nf = 0;
  for (int k = 0; k < m; k++) {
    char nm[32]; scanf("%31s", nm);
    const H *h = NULL;
    for (size_t t = 0; t < sizeof TAB / sizeof *TAB; t++) if (!strcmp(TAB[t].name, nm)) h = &TAB[t];
    if (!h) { printf("%s: unknown hypothesis\n", nm); continue; }
    int bad = -1;
    for (int i = 0; i < n; i++) if (h->fn(in[i]) != out[i]) { bad = i; break; }
    if (bad < 0) { printf("%s: consistent\n", nm); strcpy(names[nf++], nm); }
    else printf("%s: refuted by #%d (in=%d expected=%d got=%d)\n", nm, bad + 1, in[bad], out[bad], h->fn(in[bad]));
  }
  if (nf == 0) puts("verdict: no hypothesis fits");
  else if (nf == 1) printf("verdict: %s fits all observations (not proven)\n", names[0]);
  else { printf("verdict: ambiguous ("); for (int i = 0; i < nf; i++) printf("%s%s", i ? ", " : "", names[i]); puts(") - more observations needed"); }
  return 0;
}
