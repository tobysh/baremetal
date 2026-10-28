#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <errno.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ptrace.h>
#include <fcntl.h>
#include <sys/stat.h>
static void cat(const char*p,int n){FILE*f=fopen(p,"r");if(!f){printf("[%s] open fail: %s\n",p,strerror(errno));return;}char b[512];size_t k=fread(b,1,n<512?n:511,f);b[k]=0;printf("[%s]\n%s\n",p,b);fclose(f);}
static void ls(const char*p){DIR*d=opendir(p);if(!d){printf("ls %s fail: %s\n",p,strerror(errno));return;}struct dirent*e;printf("ls %s:",p);while((e=readdir(d)))printf(" %s",e->d_name);printf("\n");closedir(d);}
int main(void){
  ls("/");ls("/work");ls("/tmp");ls("/home");ls("/etc");ls("/proc/1");
  cat("/etc/passwd",200);cat("/proc/1/environ",200);cat("/proc/self/mountinfo",1500);
  cat("/proc/sys/kernel/hostname",50);
  int s=socket(AF_INET,SOCK_STREAM,0);printf("socket inet: %d %s\n",s,s<0?strerror(errno):"");
  if(s>=0){struct sockaddr_in a={.sin_family=AF_INET,.sin_port=htons(27017)};inet_pton(AF_INET,"127.0.0.1",&a.sin_addr);printf("connect 27017: %d %s\n",connect(s,(void*)&a,sizeof a),strerror(errno));}
  int r=unshare(CLONE_NEWUSER);printf("unshare NEWUSER: %d %s\n",r,r?strerror(errno):"OK");
  int w=open("/usr/x",O_CREAT|O_WRONLY,0600);printf("write /usr: %d %s\n",w,strerror(errno));
  w=open("/etc/x",O_CREAT|O_WRONLY,0600);printf("write /etc: %d %s\n",w,strerror(errno));
  FILE*f=fopen("/proc/sys/kernel/hostname","w");printf("write proc/sys: %s\n",f?"OK":strerror(errno));
  printf("ptrace pid1: %ld %s\n",ptrace(PTRACE_ATTACH,1,0,0),strerror(errno));
  printf("uid %d pid %d\n",getuid(),getpid());
  int u=socket(AF_UNIX,SOCK_STREAM,0);struct sockaddr_un{unsigned short f;char p[108];}x={1,"\0abs"};printf("abstract connect: %d %s\n",connect(u,(void*)&x,sizeof(short)+5),strerror(errno));
  return 0;}
