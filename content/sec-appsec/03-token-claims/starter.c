#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
/* b64u_decode() and toy_mac_hex() from the previous lessons are below. */

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

int main(void){puts("TODO");return 0;}
