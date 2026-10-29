#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
static int b64v(int c){ if(c>='A'&&c<='Z')return c-'A'; if(c>='a'&&c<='z')return c-'a'+26; if(c>='0'&&c<='9')return c-'0'+52; if(c=='-')return 62; if(c=='_')return 63; return -1; }
/* Strict unpadded base64url. Returns 0 on success, -1 if invalid. */
static int b64u_decode(const char *in,size_t n,uint8_t *out,size_t cap,size_t *len){
  if(n%4==1)return -1;
  size_t o=0; uint32_t acc=0; int bits=0;
  for(size_t i=0;i<n;i++){ int v=b64v((unsigned char)in[i]); if(v<0)return -1; acc=(acc<<6)|(uint32_t)v; bits+=6;
    if(bits>=8){ bits-=8; if(o>=cap)return -1; out[o++]=(uint8_t)(acc>>bits); acc&=(1u<<bits)-1u; } }
  if(acc!=0)return -1;
  *len=o; return 0;
}

/* Toy MAC for teaching only: FNV-1a 64 over key, 0, message, 0, key. NOT secure; use a real HMAC. */
static void toy_mac_hex(const char *key,const char *msg,size_t mlen,char out[17]){
  uint64_t h=0xcbf29ce484222325ULL; size_t kl=strlen(key);
  for(size_t i=0;i<kl;i++){h^=(uint8_t)key[i];h*=0x100000001b3ULL;}
  h^=0;h*=0x100000001b3ULL;
  for(size_t i=0;i<mlen;i++){h^=(uint8_t)msg[i];h*=0x100000001b3ULL;}
  h^=0;h*=0x100000001b3ULL;
  for(size_t i=0;i<kl;i++){h^=(uint8_t)key[i];h*=0x100000001b3ULL;}
  snprintf(out,17,"%016llx",(unsigned long long)h);
}

static int kv(char *s,char k[16][32],char v[16][96],int *n){
  *n=0;
  for(char *seg=s;seg;){
    char *semi=strchr(seg,';'); if(semi)*semi=0;
    char *eq=strchr(seg,'='); if(!eq||eq==seg||*n>=16||(size_t)(eq-seg)>=32||strlen(eq+1)>=96)return 0;
    size_t kl=(size_t)(eq-seg); memcpy(k[*n],seg,kl); k[*n][kl]=0;
    for(int i=0;i<*n;i++) if(!strcmp(k[i],k[*n]))return 0;
    strcpy(v[*n],eq+1); (*n)++;
    seg=semi?semi+1:NULL;
  }
  return 1;
}
static const char *get(char k[16][32],char v[16][96],int n,const char *key){ for(int i=0;i<n;i++) if(!strcmp(k[i],key))return v[i]; return NULL; }
static int num(const char *s,long long *o){ size_t l=strlen(s); if(l<1||l>18)return 0; long long x=0; for(size_t i=0;i<l;i++){ if(s[i]<'0'||s[i]>'9')return 0; x=x*10+(s[i]-'0'); } *o=x; return 1; }
int main(void){
  char key[128],line[2048]; if(!fgets(key,sizeof key,stdin))return 0; key[strcspn(key,"\n")]=0;
  long long now,skew; char aud[64],iss[64];
  if(!fgets(line,sizeof line,stdin)||sscanf(line,"%lld %lld %63s %63s",&now,&skew,aud,iss)!=4)return 0;
  int n=0;
  while(fgets(line,sizeof line,stdin)){
    line[strcspn(line,"\n")]=0; n++;
    char *d1=strchr(line,'.'); char *d2=d1?strchr(d1+1,'.'):NULL;
    if(!d1||!d2||strchr(d2+1,'.')||d1==line||d2==d1+1){ printf("token %d: malformed\n",n); continue; }
    uint8_t hb[512],pb[512]; size_t hl,pl;
    if(b64u_decode(line,(size_t)(d1-line),hb,sizeof hb-1,&hl)||b64u_decode(d1+1,(size_t)(d2-d1-1),pb,sizeof pb-1,&pl)){ printf("token %d: malformed\n",n); continue; }
    hb[hl]=0; pb[pl]=0;
    char hk[16][32],hv[16][96],pk[16][32],pv[16][96]; int hn,pn;
    if(!kv((char*)hb,hk,hv,&hn)||!kv((char*)pb,pk,pv,&pn)){ printf("token %d: malformed\n",n); continue; }
    const char *alg=get(hk,hv,hn,"alg");
    if(!alg||strcmp(alg,"HS256")){ printf("token %d: bad-alg\n",n); continue; }
    char want[17]; toy_mac_hex(key,line,(size_t)(d2-line),want);
    const char *sig=d2+1; size_t sl=strlen(sig); unsigned diff=(unsigned)(sl!=16);
    for(size_t i=0;i<16;i++) diff|=(unsigned)(want[i]^(i<sl?sig[i]:0));
    if(diff){ printf("token %d: bad-signature\n",n); continue; }
    const char *e=get(pk,pv,pn,"exp"),*nb=get(pk,pv,pn,"nbf");
    if(!e){ printf("token %d: missing-exp\n",n); continue; }
    long long ex=0,nbf=0;
    if(!num(e,&ex)||(nb&&!num(nb,&nbf))){ printf("token %d: bad-claim\n",n); continue; }
    if(now>ex+skew){ printf("token %d: expired\n",n); continue; }
    if(nb&&now+skew<nbf){ printf("token %d: not-yet-valid\n",n); continue; }
    const char *a=get(pk,pv,pn,"aud"),*i=get(pk,pv,pn,"iss"),*s=get(pk,pv,pn,"sub");
    if(!a||strcmp(a,aud)){ printf("token %d: wrong-audience\n",n); continue; }
    if(!i||strcmp(i,iss)){ printf("token %d: wrong-issuer\n",n); continue; }
    if(!s){ printf("token %d: missing-sub\n",n); continue; }
    printf("token %d: ok sub=%s\n",n,s);
  }
  return 0;
}
