#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
int parse_request(const char *line,char *method,size_t mcap,char *path,size_t pcap){
 if(!line||!method||!path)return 0;
 sscanf(line,"%s %s",method,path);
 return 1;
}
int main(void){return 0;}
