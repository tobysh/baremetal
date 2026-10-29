#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
static uint16_t be16(const uint8_t *p){return (uint16_t)((unsigned)p[0]<<8|p[1]);}
static uint32_t be32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
typedef struct {uint8_t dst[6],src[6];uint16_t type;size_t payload;} Ethernet;
int parse_ethernet(const uint8_t *p,size_t n,Ethernet *out){
 if(!p||!out||n<14)return -1;Ethernet e={0};memcpy(e.dst,p,6);memcpy(e.src,p+6,6);e.type=be16(p+12);e.payload=14;
 if(e.type==0x8100){if(n<18)return -1;e.type=be16(p+16);e.payload=18;}*out=e;return 0;
}
typedef struct {uint32_t src,dst;uint16_t total;uint8_t protocol;size_t header;int fragmented;} IPv4;
int parse_ipv4(const uint8_t *p,size_t n,IPv4 *out){
 if(!p||!out||n<20 || p[0]>>4!=4)return -1;size_t h=(p[0]&15)*4;uint16_t total=be16(p+2);
 if(h<20 || h>n || total<h || total>n)return -1;
 *out=(IPv4){be32(p+12),be32(p+16),total,p[9],h,(be16(p+6)&0x3fff)!=0};return 0;
}
typedef struct {uint16_t src,dst;uint32_t seq,ack;uint8_t flags;size_t header,payload;} TCP;
int parse_tcp(const uint8_t *p,size_t n,TCP *out){
 if(!p||!out||n<20)return -1;size_t h=(p[12]>>4)*4;if(h<20||h>n)return -1;
 *out=(TCP){be16(p),be16(p+2),be32(p+4),be32(p+8),p[13],h,n-h};return 0;
}
typedef struct {uint16_t src,dst;size_t payload;} UDP;
int parse_udp(const uint8_t *p,size_t n,UDP *out){
 if(!p||!out||n<8)return -1;size_t len=be16(p+4);if(len<8||len>n)return -1;*out=(UDP){be16(p),be16(p+2),len-8};return 0;
}

const char *classify(const uint8_t *frame,size_t n){Ethernet e;if(parse_ethernet(frame,n,&e))return "invalid";if(e.type==0x0806)return "arp";if(e.type!=0x0800)return "other";
 IPv4 ip;if(parse_ipv4(frame+e.payload,n-e.payload,&ip))return "invalid";if(ip.fragmented)return "ipv4/fragment";const uint8_t *p=frame+e.payload+ip.header;size_t len=ip.total-ip.header;
 if(ip.protocol==6){TCP t;return parse_tcp(p,len,&t)?"invalid":"ipv4/tcp";}
 if(ip.protocol==17){UDP u;if(parse_udp(p,len,&u))return "invalid";return u.src==53||u.dst==53?"ipv4/udp/dns-port":"ipv4/udp";}return "ipv4/other";}
int main(void){return 0;}
