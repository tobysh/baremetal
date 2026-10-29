#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
typedef struct {const char *type;size_t offset,length;} Carved;
size_t carve(const uint8_t *p,size_t n,Carved *out,size_t max){size_t count=0,i=0;
 while(i<n && count<max){size_t end=0;const char *type=NULL;
  if(n-i>=3 && !memcmp(p+i,"\377\330\377",3)){for(size_t j=i+3;j+1<n;j++)if(p[j]==255&&p[j+1]==217){end=j+2;type="JPEG";break;}}
  else if(n-i>=8 && !memcmp(p+i,"\211PNG\r\n\032\n",8)){for(size_t j=i+8;n-j>=8;j++)if(!memcmp(p+j,"IEND",4)){end=j+8;type="PNG";break;}}
  if(end){out[count++]=(Carved){type,i,end-i};i=end;}else i++;
 }return count;}
int main(void){return 0;}
