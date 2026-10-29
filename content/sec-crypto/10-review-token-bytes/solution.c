#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
static int digit(char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1;}
int token_equal(const char *hex,const uint8_t *expected,size_t n){
 if(!hex || (n&&!expected) || n>SIZE_MAX/2 || strlen(hex)!=2*n)return 0;
 for(size_t i=0;i<2*n;i++)if(digit(hex[i])<0)return 0;
 unsigned diff=0;for(size_t i=0;i<n;i++)diff|=((digit(hex[2*i])<<4)|digit(hex[2*i+1]))^expected[i];return diff==0;
}
int main(void){return 0;}
