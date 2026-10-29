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

int main(void){
  char key[128],line[2048]; if(!fgets(key,sizeof key,stdin))return 0; key[strcspn(key,"\n")]=0;
  int n=0;
  while(fgets(line,sizeof line,stdin)){
    line[strcspn(line,"\n")]=0; n++;
    const char *verdict="ok";
    char *d1=strchr(line,'.'); char *d2=d1?strchr(d1+1,'.'):NULL;
    if(!d1||!d2||strchr(d2+1,'.')||d1==line||d2==d1+1){ printf("token %d: malformed\n",n); continue; }
    uint8_t hb[512],pb[512]; size_t hl,pl;
    if(b64u_decode(line,(size_t)(d1-line),hb,sizeof hb-1,&hl)||b64u_decode(d1+1,(size_t)(d2-d1-1),pb,sizeof pb-1,&pl)){ printf("token %d: malformed\n",n); continue; }
    hb[hl]=0; int bad=0,alg=-1; char keys[16][32]; int nk=0; char algv[64]="";
    for(char *seg=(char*)hb;seg&&!bad;){
      char *semi=strchr(seg,';'); if(semi)*semi=0;
      char *eq=strchr(seg,'='); if(!eq||eq==seg||nk>=16||(size_t)(eq-seg)>=32){bad=1;break;}
      size_t kl=(size_t)(eq-seg); char kk[32]; memcpy(kk,seg,kl); kk[kl]=0;
      for(int i=0;i<nk;i++) if(!strcmp(keys[i],kk))bad=1;
      strcpy(keys[nk++],kk); if(!strcmp(kk,"alg")){alg=1;snprintf(algv,64,"%s",eq+1);}
      seg=semi?semi+1:NULL;
    }
    if(bad){ printf("token %d: malformed\n",n); continue; }
    if(alg<0||strcmp(algv,"HS256")){ printf("token %d: bad-alg\n",n); continue; }
    char want[17]; toy_mac_hex(key,line,(size_t)(d2-line),want);
    const char *sig=d2+1; size_t sl=strlen(sig); unsigned diff=(unsigned)(sl!=16);
    for(size_t i=0;i<16;i++) diff|=(unsigned)(want[i]^(i<sl?sig[i]:0));
    if(diff)verdict="bad-signature";
    printf("token %d: %s\n",n,verdict);
  }
  return 0;
}
