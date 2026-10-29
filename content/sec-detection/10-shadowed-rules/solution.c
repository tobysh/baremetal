#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
typedef struct { int allow; int proto; /* 0 any, 6 tcp, 17 udp */ uint32_t sn, sm, dn, dm; int plo, phi; } Rule;
static int parse_ip(const char *s, uint32_t *out) {
  unsigned a, b, c, d; char tail;
  if (sscanf(s, "%u.%u.%u.%u%c", &a, &b, &c, &d, &tail) != 4 || a > 255 || b > 255 || c > 255 || d > 255) return 0;
  *out = (a << 24) | (b << 16) | (c << 8) | d; return 1;
}
/* "any", "a.b.c.d" or "a.b.c.d/len" -> normalised network and mask. Note: a shift by 32 is undefined in C. */
static int parse_net(const char *s, uint32_t *net, uint32_t *mask) {
  if (!strcmp(s, "any")) { *net = 0; *mask = 0; return 1; }
  char buf[64]; snprintf(buf, sizeof buf, "%s", s);
  int len = 32; char *sl = strchr(buf, '/');
  if (sl) { *sl = 0; len = atoi(sl + 1); }
  uint32_t ip; if (!parse_ip(buf, &ip) || len < 0 || len > 32) return 0;
  *mask = len == 0 ? 0 : 0xFFFFFFFFu << (32 - len); *net = ip & *mask; return 1;
}
static int parse_proto(const char *s) { return !strcmp(s, "tcp") ? 6 : !strcmp(s, "udp") ? 17 : 0; }
static void parse_ports(const char *s, int *lo, int *hi) {
  if (!strcmp(s, "any")) { *lo = 0; *hi = 65535; return; }
  char *dash = strchr(s, '-');
  *lo = atoi(s); *hi = dash ? atoi(dash + 1) : *lo;
}
static int parse_rule(const char *line, Rule *r) {
  char act[16], pr[16], s[64], d[64], po[32];
  if (sscanf(line, "%15s %15s %63s %63s %31s", act, pr, s, d, po) != 5) return 0;
  r->allow = !strcmp(act, "allow"); r->proto = parse_proto(pr);
  if (!parse_net(s, &r->sn, &r->sm) || !parse_net(d, &r->dn, &r->dm)) return 0;
  parse_ports(po, &r->plo, &r->phi); return 1;
}

static int contains(uint32_t an,uint32_t am,uint32_t bn,uint32_t bm){ return (am&bm)==am && (bn&am)==an; }
int main(void){
  Rule R[64]; int nr=0; char line[256];
  while(nr<64&&fgets(line,sizeof line,stdin)) if(parse_rule(line,&R[nr]))nr++;
  int any=0;
  for(int j=0;j<nr;j++) for(int i=0;i<j;i++){
    Rule *a=&R[i],*b=&R[j];
    if(a->proto&&a->proto!=b->proto)continue;
    if(!contains(a->sn,a->sm,b->sn,b->sm)||!contains(a->dn,a->dm,b->dn,b->dm))continue;
    if(a->plo>b->plo||a->phi<b->phi)continue;
    printf("rule %d shadowed by rule %d (%s)\n",j+1,i+1,a->allow==b->allow?"redundant":"conflict"); any=1; break;
  }
  if(!any)puts("no shadowed rules");
  return 0;
}
