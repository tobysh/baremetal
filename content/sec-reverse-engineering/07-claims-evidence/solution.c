#include <stdio.h>
#include <string.h>
typedef struct { char id[16]; char text[128]; int st, dy, rp, cst, cdy, crp; } Claim;
int main(void){
  static Claim C[64]; int nc = 0; char line[256];
  while (fgets(line, sizeof line, stdin)) {
    line[strcspn(line, "\r\n")] = 0;
    char *f[4]; int nf = 0; char *p = line;
    while (nf < 4) { f[nf++] = p; char *bar = strchr(p, '|'); if (!bar) break; *bar = 0; p = bar + 1; }
    if (nf == 2 && f[0][0] == 'C' && nc < 64) { snprintf(C[nc].id, 16, "%s", f[0]); snprintf(C[nc].text, 128, "%s", f[1]); nc++; }
    else if (nf == 4 && f[0][0] == 'E') {
      int ci = -1; for (int i = 0; i < nc; i++) if (!strcmp(C[i].id, f[2])) ci = i;
      if (ci < 0) { printf("warning: evidence %s references unknown claim %s\n", f[0], f[2]); continue; }
      int con = !strcmp(f[3], "contradicts");
      Claim *c = &C[ci];
      if (!strcmp(f[1], "static")) { if (con) c->cst = 1; else c->st = 1; }
      else if (!strcmp(f[1], "dynamic")) { if (con) c->cdy = 1; else c->dy = 1; }
      else if (!strcmp(f[1], "report")) { if (con) c->crp = 1; else c->rp = 1; }
    }
  }
  int conf = 0, sup = 0, asm_ = 0, ref = 0, dis = 0;
  for (int i = 0; i < nc; i++) {
    Claim *c = &C[i]; const char *lab;
    if (c->cst || c->cdy) { lab = "refuted"; ref++; }
    else if (c->crp) { lab = "disputed"; dis++; }
    else if (c->st && c->dy) { lab = "confirmed"; conf++; }
    else if (c->st) { lab = "supported (static)"; sup++; }
    else if (c->dy) { lab = "supported (dynamic)"; sup++; }
    else { lab = "assumption"; asm_++; }
    printf("%s: %s - %s\n", c->id, lab, c->text);
  }
  printf("confirmed=%d supported=%d assumption=%d refuted=%d disputed=%d\n", conf, sup, asm_, ref, dis);
  return 0;
}
