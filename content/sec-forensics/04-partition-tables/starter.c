#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
typedef struct {int bootable;uint8_t type;uint32_t start_lba,sectors;} Partition;
int parse_mbr(const uint8_t *p,Partition out[4]){(void)p;(void)out;return -1;}
const char *type_name(uint8_t t){(void)t;return "Unknown";}
int main(void){return 0;}
