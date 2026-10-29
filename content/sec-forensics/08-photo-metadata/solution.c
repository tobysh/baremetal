#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
typedef struct {char make[64],model[64];uint32_t width,height;char datetime[32];} Meta;
static uint32_t word(const uint8_t *p,int b,int big){uint32_t v=0;if(big){for(int i=0;i<b;i++)v=(v<<8)|p[i];}else{for(int i=b-1;i>=0;i--)v=(v<<8)|p[i];}return v;}
int read_meta(const uint8_t *p,size_t n,Meta *out){if(n<8 || !out)return -1;int big=p[0]=='M'&&p[1]=='M';if(!big && !(p[0]=='I'&&p[1]=='I'))return -1;
 if(word(p+2,2,big)!=42)return -1;size_t off=word(p+4,4,big);if(off>n || n-off<2)return -1;size_t count=word(p+off,2,big);off+=2;if(count>(n-off)/12)return -1;Meta m={0};
 for(size_t i=0;i<count;i++){const uint8_t *e=p+off+12*i;unsigned tag=word(e,2,big),type=word(e+2,2,big);size_t c=word(e+4,4,big);uint32_t v=word(e+8,4,big);char *dst=NULL;size_t cap=0;
  if(tag==0x10f){dst=m.make;cap=sizeof m.make;}else if(tag==0x110){dst=m.model;cap=sizeof m.model;}else if(tag==0x132){dst=m.datetime;cap=sizeof m.datetime;}
  if(dst && type==2){if(!c || c>cap)return -1;const uint8_t *s=e+8;if(c>4){if(v>n||c>n-v)return -1;s=p+v;}if(s[c-1]!=0 || memchr(s,0,c)==NULL)return -1;memcpy(dst,s,c);}
  if((tag==0x100||tag==0x101)&&(type==3||type==4)){if(c!=1)return -1;uint32_t val=type==3?word(e+8,2,big):v;if(tag==0x100)m.width=val;else m.height=val;}
 }*out=m;return 0;}
static int leap(int y){return y%4==0 && (y%100!=0 || y%400==0);}
long long to_unix(const char *s){
 if(!s || strlen(s)!=19)return -1;
 for(int i=0;i<19;i++){if(i==4||i==7||i==13||i==16){if(s[i]!=':')return -1;}else if(i==10){if(s[i]!=' ')return -1;}else if(s[i]<'0'||s[i]>'9')return -1;}
 int y,mo,d,h,mi,se;if(sscanf(s,"%4d:%2d:%2d %2d:%2d:%2d",&y,&mo,&d,&h,&mi,&se)!=6)return -1;
 const int days[]={31,28,31,30,31,30,31,31,30,31,30,31};if(y<1970||y>2100||mo<1||mo>12||d<1||d>days[mo-1]+(mo==2&&leap(y))||h>23||mi>59||se>59)return -1;
 long long total=0;for(int yr=1970;yr<y;yr++)total+=365+leap(yr);for(int m=1;m<mo;m++)total+=days[m-1]+(m==2&&leap(y));total+=d-1;return ((total*24+h)*60+mi)*60+se;
}
int main(void){return 0;}
