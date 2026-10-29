#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
static int b64v(int c){ if(c>='A'&&c<='Z')return c-'A'; if(c>='a'&&c<='z')return c-'a'+26; if(c>='0'&&c<='9')return c-'0'+52; if(c=='-')return 62; if(c=='_')return 63; return -1; }
/* Strict unpadded base64url. Returns 0 on success, -1 if invalid. */
int b64u_decode(const char *in,size_t n,uint8_t *out,size_t cap,size_t *len){
  if(n%4==1)return -1;
  size_t o=0; uint32_t acc=0; int bits=0;
  for(size_t i=0;i<n;i++){ int v=b64v((unsigned char)in[i]); if(v<0)return -1; acc=(acc<<6)|(uint32_t)v; bits+=6;
    if(bits>=8){ bits-=8; if(o>=cap)return -1; out[o++]=(uint8_t)(acc>>bits); acc&=(1u<<bits)-1u; } }
  if(acc!=0)return -1;
  *len=o; return 0;
}
int main(void){return 0;}
