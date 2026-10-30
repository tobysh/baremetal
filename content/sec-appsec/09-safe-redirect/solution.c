#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
/* Safe local redirect: one leading slash, no backslash, no control or space bytes (raw or %-escaped). */
int safe_redirect(const char *t){
  if(!t)return 0; size_t n=strlen(t); if(n==0||n>2048||t[0]!='/')return 0;
  for(size_t i=0;i<n;i++){
    unsigned c=(unsigned char)t[i];
    if(c<=0x20||c==0x7f||c=='\\')return 0;
    if(c=='%'){
      if(i+2>=n)return 0;
      int a,b; unsigned char x=(unsigned char)t[i+1],y=(unsigned char)t[i+2];
      a=(x>='0'&&x<='9')?x-'0':(x>='a'&&x<='f')?x-'a'+10:(x>='A'&&x<='F')?x-'A'+10:-1;
      b=(y>='0'&&y<='9')?y-'0':(y>='a'&&y<='f')?y-'a'+10:(y>='A'&&y<='F')?y-'A'+10:-1;
      if(a<0||b<0)return 0;
      unsigned v=(unsigned)(a*16+b);
      if(v<0x20||v==0x7f||v==0x5c)return 0;
      if(i==1&&v==0x2f)return 0;
    }
  }
  if(n>=2&&(t[1]=='/'))return 0;
  return 1;
}
int main(void){return 0;}
