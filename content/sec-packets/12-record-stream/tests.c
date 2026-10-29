static uint32_t st=2024;static uint32_t rnd(void){st=st*1664525u+1013904223u;return st>>8;}
static int ref(const uint8_t*b,size_t n,size_t*rc,size_t*cons){unsigned long long p=0;*rc=0;*cons=0;
 for(;;){if(p==n)return 0;if(n-p<5)return 1;unsigned t=b[p],vh=b[p+1],vl=b[p+2];unsigned long long l=b[p+3]*256ull+b[p+4];
  if(!(t>=20&&t<=23&&vh==3&&vl>=1&&vl<=4&&l>=1&&l<=16384))return -1;
  if(p+5+l>n)return 1;p+=5+l;(*rc)++;*cons=p;}}
int main(void){
 static uint8_t buf[5000];
 for(int it=0;it<500;it++){
  size_t n=0;int recs=rnd()%5;size_t hdr[8];int nh=0;
  for(int r=0;r<recs;r++){size_t l=1+rnd()%150;hdr[nh++]=n;buf[n++]=(uint8_t)(20+rnd()%4);buf[n++]=3;buf[n++]=(uint8_t)(1+rnd()%4);buf[n++]=(uint8_t)(l>>8);buf[n++]=(uint8_t)l;for(size_t i=0;i<l;i++)buf[n++]=(uint8_t)rnd();}
  int mode=rnd()%6;
  if(mode==1&&n)n-=1+rnd()%(n<9?n:9);
  if(mode==2&&nh){size_t h=hdr[rnd()%nh];buf[h+rnd()%3]^=(uint8_t)(1+rnd()%255);}
  if(mode==3&&nh){size_t h=hdr[rnd()%nh];buf[h+3]=0;buf[h+4]=0;}
  if(mode==4&&nh){size_t h=hdr[rnd()%nh];buf[h+3]=0x7f;buf[h+4]=0xff;}
  if(mode==5){for(int e=0;e<(int)(rnd()%5);e++)buf[n++]=(uint8_t)rnd();}
  uint8_t*c=malloc(n?n:1);memcpy(c,buf,n);
  size_t wr,wc,gr=77,gc=77;int want=ref(c,n,&wr,&wc);int got=tls_walk(c,n,&gr,&gc);
  CHECK("generated streams",got==want&&gr==wr&&gc==wc,"iteration %d (%zu bytes): returned %d records %zu consumed %zu, want %d %zu %zu",it,n,got,gr,gc,want,wr,wc);
  free(c);}
 uint8_t hs[]={22,3,3,0,0};size_t r1=9,c1=9;
 CHECK("zero length is invalid",tls_walk(hs,5,&r1,&c1)==-1&&r1==0&&c1==0,"");
 uint8_t big[]={23,3,3,0x40,0x01};
 CHECK("length above 16384",tls_walk(big,5,NULL,NULL)==-1,"");
 uint8_t part[]={23,3,3,0x40,0x00,1,2,3};
 CHECK("header is judged before the body arrives",tls_walk(part,8,&r1,&c1)==1&&r1==0&&c1==0,"");
 CHECK("partial header",tls_walk(part,3,&r1,&c1)==1,"");
 CHECK("NULL buffer",tls_walk(NULL,4,&r1,&c1)==-1&&r1==0&&c1==0&&tls_walk(NULL,0,&r1,&c1)==0,"");
 return 0;}
