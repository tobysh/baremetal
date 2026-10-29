#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
typedef struct {char ip[64],users[256][64],accepted[64];int fails,count;} Source;
static Source src[256];
int main(void){char line[1024];int n=0;
 while(fgets(line,sizeof line,stdin)){if(!strstr(line,"sshd["))continue;char *p=strstr(line," Failed password for ");int failed=p!=NULL;if(p)p+=21;else{p=strstr(line," Accepted password for ");if(!p)continue;p+=23;}
  if(failed && !strncmp(p,"invalid user ",13))p+=13;char user[64],ip[64];if(sscanf(p,"%63s from %63s",user,ip)!=2)continue;int j=0;while(j<n && strcmp(src[j].ip,ip))j++;if(j==n){if(n==256)continue;strcpy(src[n++].ip,ip);}
  if(failed){src[j].fails++;int k=0;while(k<src[j].count && strcmp(src[j].users[k],user))k++;if(k==src[j].count && k<256)strcpy(src[j].users[src[j].count++],user);}
  else if(src[j].fails>=5 && !src[j].accepted[0])strcpy(src[j].accepted,user);
 }
 int order[256],m=0;for(int i=0;i<n;i++)if(src[i].fails>=5)order[m++]=i;
 for(int i=1;i<m;i++){int v=order[i],j=i;while(j && (src[order[j-1]].fails<src[v].fails || (src[order[j-1]].fails==src[v].fails && strcmp(src[order[j-1]].ip,src[v].ip)>0))){order[j]=order[j-1];j--;}order[j]=v;}
 if(!m)puts("no brute force detected");for(int i=0;i<m;i++){Source *s=&src[order[i]];printf("%s failures=%d users=%d",s->ip,s->fails,s->count);if(s->accepted[0])printf(" ACCEPTED as %s",s->accepted);putchar('\n');}return 0;}
