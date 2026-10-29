#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
size_t log_escape(char *out,size_t cap,const char *in){
 static const char hx[]="0123456789abcdef";
 if(!in)in="";
 size_t total=0,w=0;int stopped=0;
 for(const unsigned char*p=(const unsigned char*)in;*p;p++){
  char piece[4];size_t l;
  if(*p=='\n'){piece[0]='\\';piece[1]='n';l=2;}
  else if(*p=='\r'){piece[0]='\\';piece[1]='r';l=2;}
  else if(*p=='\\'){piece[0]='\\';piece[1]='\\';l=2;}
  else if(*p<0x20||*p==0x7f){piece[0]='\\';piece[1]='x';piece[2]=hx[*p>>4];piece[3]=hx[*p&15];l=4;}
  else{piece[0]=(char)*p;l=1;}
  total+=l;
  if(!out||cap==0)continue;
  if(!stopped&&w+l<cap){memcpy(out+w,piece,l);w+=l;}else stopped=1;
 }
 if(out&&cap)out[w]=0;
 return total;
}
int main(void){return 0;}
