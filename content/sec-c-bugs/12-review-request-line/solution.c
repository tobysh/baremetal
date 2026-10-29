#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
int parse_request(const char *line,char *method,size_t mcap,char *path,size_t pcap){
 if(method&&mcap)method[0]=0;if(path&&pcap)path[0]=0;
 if(!line||!method||!path||!mcap||!pcap)return 0;
 const char*s1=strchr(line,' ');if(!s1)return 0;
 size_t ml=(size_t)(s1-line);
 if(!((ml==3&&!memcmp(line,"GET",3))||(ml==4&&(!memcmp(line,"HEAD",4)||!memcmp(line,"POST",4)))))return 0;
 const char*t=s1+1;const char*s2=strchr(t,' ');if(!s2)return 0;
 size_t tl=(size_t)(s2-t);
 if(tl==0||t[0]!='/')return 0;
 for(size_t i=0;i<tl;i++){unsigned char c=(unsigned char)t[i];if(c<0x21||c>0x7e)return 0;}
 const char*v=s2+1;if(strcmp(v,"HTTP/1.0")&&strcmp(v,"HTTP/1.1"))return 0;
 for(size_t i=0;i<tl;){size_t j=i;while(j<tl&&t[j]!='/')j++;if(j-i==2&&t[i]=='.'&&t[i+1]=='.')return 0;i=j+1;}
 if(ml>=mcap||tl>=pcap)return 0;
 memcpy(method,line,ml);method[ml]=0;memcpy(path,t,tl);path[tl]=0;return 1;
}
int main(void){return 0;}
