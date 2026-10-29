static uint32_t st=12345;static uint32_t rnd(void){st=st*1664525u+1013904223u;return st>>8;}
static int ref(const uint8_t*b,size_t n,uint8_t ty,size_t*o,size_t*v){unsigned long long p=0;
 while(p<n){if(n-p<3)return -1;unsigned long long l=b[p+1]*256ull+b[p+2];if(p+3+l>n)return -1;
  if(b[p]==ty){*o=p+3;*v=l;return 1;}p+=3+l;}return 0;}
int main(void){
 for(int it=0;it<500;it++){
  uint8_t tmp[300];size_t n=0;int recs=rnd()%6;
  for(int r=0;r<recs;r++){size_t l=rnd()%20;if(n+3+l>sizeof tmp)break;tmp[n++]=1+rnd()%5;tmp[n++]=0;tmp[n++]=(uint8_t)l;for(size_t i=0;i<l;i++)tmp[n++]=(uint8_t)rnd();}
  int mode=rnd()%4;
  if(mode==1&&n)n-=1+rnd()%(n<4?n:4);
  if(mode==2&&n>=3){size_t k=rnd()%n;tmp[k]=(uint8_t)rnd();}
  if(mode==3&&n>=3){tmp[1]=0xff;tmp[2]=0xff;}
  uint8_t *b=malloc(n?n:1);memcpy(b,tmp,n);
  uint8_t ty=(uint8_t)(1+rnd()%6);
  size_t o=77,v=77,ro=0,rv=0;int want=ref(b,n,ty,&ro,&rv);if(want!=1){ro=0;rv=0;}
  int got=tlv_find(b,n,ty,&o,&v);
  CHECK("generated buffers",got==want&&o==ro&&v==rv,"iteration %d, length %zu, type %u: got %d (off %zu len %zu), want %d (off %zu len %zu)",it,n,ty,got,o,v,want,ro,rv);
  free(b);}
 size_t o=9,v=9;
 CHECK("NULL buffer, empty",tlv_find(NULL,0,1,&o,&v)==0&&o==0&&v==0,"");
 CHECK("NULL buffer, claimed bytes",tlv_find(NULL,5,1,&o,&v)==-1,"");
 uint8_t one[]={7,0,0};
 CHECK("optional outputs",tlv_find(one,3,7,NULL,NULL)==1&&tlv_find(one,3,8,NULL,NULL)==0,"");
 return 0;}
