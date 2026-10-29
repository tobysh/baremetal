#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
typedef struct {int bootable;uint8_t type;uint32_t start_lba,sectors;} Partition;
static uint32_t le32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
int parse_mbr(const uint8_t *p,Partition out[4]){if(p[510]!=0x55||p[511]!=0xaa)return -1;int count=0;
 for(int i=0;i<4;i++){const uint8_t *e=p+446+16*i;uint32_t sectors=le32(e+12);if(e[4]&&sectors)out[count++]=(Partition){e[0]==0x80,e[4],le32(e+8),sectors};}return count;}
const char *type_name(uint8_t t){switch(t){case 7:return "NTFS/exFAT";case 11:return "FAT32 (CHS)";case 12:return "FAT32 (LBA)";case 0x82:return "Linux swap";case 0x83:return "Linux";case 5:return "Extended";case 0xee:return "GPT protective";case 0xef:return "EFI System";default:return "Unknown";}}
int main(void){return 0;}
