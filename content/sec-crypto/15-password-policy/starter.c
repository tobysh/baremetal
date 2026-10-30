#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
int main(void){char l[512];int n=0;while(fgets(l,sizeof l,stdin))printf("%d: ok\n",++n);return 0;}
