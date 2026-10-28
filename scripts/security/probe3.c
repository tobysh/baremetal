#define _GNU_SOURCE
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <stdlib.h>
static void st(const char*n,long r){int e=errno;printf("%-28s -> %ld %s\n",n,r,r<0?strerror(e):"");}
int main(int argc,char**argv){
  if(argc>1&&!strcmp(argv[1],"daemon")){ if(fork()==0){setsid(); if(fork()==0){ sleep(60); _exit(0);} _exit(0);} sleep(30); return 0; }
  char b[4096]; int fd=open("/proc/self/status",O_RDONLY); int n=read(fd,b,sizeof b-1); b[n]=0;
  char*p=strstr(b,"Seccomp:"); printf("%.12s\n",p); p=strstr(b,"CapEff"); printf("%.30s\n",p);
  // dangerous syscalls availability
  st("io_uring_setup",syscall(425,4,(void*)b));
  st("bpf",syscall(321,0,b,0));
  st("perf_event_open",syscall(298,b,0,-1,-1,0));
  st("userfaultfd",syscall(323,0));
  st("keyctl",syscall(250,0,0,0,0,0));
  st("unshare(NEWUSER|NEWNS|NEWNET)",unshare(CLONE_NEWUSER|CLONE_NEWNS|CLONE_NEWNET));
  printf("uid after unshare %d, CapEff:",getuid()); fd=open("/proc/self/status",O_RDONLY); n=read(fd,b,sizeof b-1); b[n]=0; p=strstr(b,"CapEff"); printf(" %.26s\n",p);
  st("mount tmpfs /mnt(nested)",mount("none","/tmp",NULL,0,NULL)); 
  st("mount tmpfs",mount("tmpfs","/tmp","tmpfs",0,"size=1m"));
  st("socket AF_NETLINK",socket(16,3,0));
  st("socket AF_PACKET",socket(17,3,0));
  st("socket NETLINK_NETFILTER",socket(16,3,12));
  // mode-000 dir trap for host-side rm
  mkdir("/work/trap",0700); int f=open("/work/trap/f",O_CREAT|O_WRONLY,0600); close(f); st("chmod 000 trap",chmod("/work/trap",0));
  // tmpfs caps: write 40x4MB to /tmp and /
  int ok=0; for(int i=0;i<40;i++){char nm[64]; sprintf(nm,"/tmp/f%d",i); int g=open(nm,O_CREAT|O_WRONLY,0600); char*m=calloc(1,4<<20); memset(m,1,4<<20); if(write(g,m,4<<20)==(4<<20))ok++; free(m); close(g);} printf("wrote %d x 4MB to /tmp\n",ok);
  ok=0; for(int i=0;i<40;i++){char nm[64]; sprintf(nm,"/f%d",i); int g=open(nm,O_CREAT|O_WRONLY,0600); char*m=calloc(1,4<<20); memset(m,1,4<<20); if(g>=0&&write(g,m,4<<20)==(4<<20))ok++; free(m); if(g>=0)close(g);} printf("wrote %d x 4MB to / (root tmpfs)\n",ok);
  ok=0; for(int i=0;i<40;i++){char nm[64]; sprintf(nm,"/dev/shm/f%d",i); int g=open(nm,O_CREAT|O_WRONLY,0600); char*m=calloc(1,4<<20); memset(m,1,4<<20); if(g>=0&&write(g,m,4<<20)==(4<<20))ok++; free(m); if(g>=0)close(g);} printf("wrote %d x 4MB to /dev/shm\n",ok);
  { int g=open("/tmp/big",O_CREAT|O_WRONLY,0600); signal(SIGXFSZ,SIG_IGN); char*m=calloc(1,5<<20); long w=write(g,m,5<<20); printf("5MB single write -> %ld (fsize cap)\n",w);} 
  void*mm=mmap(0,1<<30,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0); printf("mmap 1GiB -> %s\n",mm==MAP_FAILED?"FAIL (as cap works)":"ok");
  return 0;}
