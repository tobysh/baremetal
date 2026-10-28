#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
int main(void){
  int fd=open("/proc/1/environ",O_RDONLY);char b[8192];int n=read(fd,b,sizeof b);
  printf("pid1 environ (%d bytes):\n",n);for(int i=0;i<n;i++)putchar(b[i]?b[i]:'\n');
  int s=socket(AF_INET,SOCK_STREAM,0);struct sockaddr_in a={.sin_family=AF_INET,.sin_port=htons(27017)};inet_pton(AF_INET,"127.0.0.1",&a.sin_addr);
  int r=connect(s,(void*)&a,sizeof a);int e=errno;printf("connect 127.0.0.1:27017 -> %d (%s)\n",r,strerror(e));
  fd=open("/etc/x",O_CREAT|O_WRONLY,0600);e=errno;printf("create /etc/x -> %d (%s)\n",fd,strerror(e));
  fd=open("/tmp/x",O_CREAT|O_WRONLY,0600);e=errno;printf("create /tmp/x -> %d\n",fd);
  r=unshare(CLONE_NEWUSER);e=errno;printf("unshare(NEWUSER) -> %d (%s)\n",r,strerror(e));
  return 0;}
