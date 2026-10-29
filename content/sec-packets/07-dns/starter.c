#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
typedef struct {uint16_t id,type,qclass;char name[254];} Question;
int dns_question(const uint8_t *p,size_t n,Question *out){(void)p;(void)n;(void)out;return -1;}
int main(void){return 0;}
