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

int main(void){
  Rule R[64]; int nr=0; char line[256];
  while(fgets(line,sizeof line,stdin)){ line[strcspn(line,"\n")]=0; if(!*line)break; if(nr<64&&parse_rule(line,&R[nr]))nr++; }
  int n=0;
  while(fgets(line,sizeof line,stdin)){
    char pr[16],s[64],d[64]; int port; if(sscanf(line,"%15s %63s %63s %d",pr,s,d,&port)!=4)continue;
    n++; int p=parse_proto(pr); uint32_t si,di; parse_ip(s,&si); parse_ip(d,&di);
    int hit=-1;
    for(int i=0;i<nr&&hit<0;i++){
      Rule *r=&R[i];
      if(r->proto&&r->proto!=p)continue;
      if((si&r->sm)!=r->sn||(di&r->dm)!=r->dn)continue;
      if(port<r->plo||port>r->phi)continue;
      hit=i;
    }
    if(hit<0)printf("packet %d: deny (default)\n",n); else printf("packet %d: %s (rule %d)\n",n,R[hit].allow?"allow":"deny",hit+1);
  }
  return 0;
}
