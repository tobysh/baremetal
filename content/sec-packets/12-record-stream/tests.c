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

 /* Boundary values: hand-built records so each rule is tested on its own. */
 static uint8_t full[16400];
 size_t rr=9,cc=9;
 memset(full,0xab,sizeof full);
 full[0]=23;full[1]=3;full[2]=3;full[3]=0x40;full[4]=0x00;
 CHECK("length 16384 with a full body",tls_walk(full,5+16384,&rr,&cc)==0&&rr==1&&cc==5+16384,"a 16384-byte record that is entirely present counts as one record, got %zu records, %zu consumed",rr,cc);
 CHECK("one byte short of a full body",tls_walk(full,5+16383,&rr,&cc)==1&&rr==0&&cc==0,"the body is 1 byte short: wait for more, with nothing consumed (got %zu records, %zu consumed)",rr,cc);
 uint8_t two[]={23,3,3,0,2,0xaa,0xbb,22,3,1,0,1,0xcc,21,3,4,0,3,1};
 CHECK("two records then a partial body",tls_walk(two,sizeof two,&rr,&cc)==1&&rr==2&&cc==13,"two complete records occupy 13 bytes and the third needs more: got %zu records, %zu consumed",rr,cc);
 CHECK("exact end after two records",tls_walk(two,13,&rr,&cc)==0&&rr==2&&cc==13,"a buffer ending on a boundary is clean: got %zu records, %zu consumed",rr,cc);
 CHECK("four trailing bytes are a partial header",tls_walk(two,11,&rr,&cc)==1&&rr==1&&cc==7,"4 bytes after one record are an incomplete header: got %zu records, %zu consumed",rr,cc);
 uint8_t badsecond[]={23,3,3,0,1,0xaa,19,3,3,0,1,0xbb};
 CHECK("second header invalid after a good record",tls_walk(badsecond,sizeof badsecond,&rr,&cc)==-1&&rr==1&&cc==6,"type 19 is invalid; the first record still counts (got %zu records, %zu consumed)",rr,cc);
 int badhdr=0;
 {const uint8_t t[]={19,24,23};const uint8_t major[]={2,4,0};
  for(int i=0;i<3;i++){uint8_t h[]={22,3,3,0,1,0};h[0]=t[i%3];if(i<2&&tls_walk(h,6,NULL,NULL)!=-1)badhdr|=1;}
  for(int i=0;i<3;i++){uint8_t h[]={22,3,3,0,1,0};h[1]=major[i];if(tls_walk(h,6,NULL,NULL)!=-1)badhdr|=2;}
  uint8_t m0[]={22,3,0,0,1,0},m5[]={22,3,5,0,1,0},m4[]={22,3,4,0,1,0},m1[]={22,3,1,0,1,0};
  if(tls_walk(m0,6,NULL,NULL)!=-1||tls_walk(m5,6,NULL,NULL)!=-1)badhdr|=4;
  if(tls_walk(m4,6,NULL,NULL)!=0||tls_walk(m1,6,NULL,NULL)!=0)badhdr|=8;}
 CHECK("type and version ranges",badhdr==0,"type must be 20..23, major 3, minor 1..4 (failure mask %d: 1 type, 2 major, 4 minor out of range, 8 valid minor rejected)",badhdr);
 uint8_t be[]={23,3,3,0x01,0x02};
 CHECK("length is big-endian",tls_walk(be,5,&rr,&cc)==1&&rr==0&&cc==0,"length 01 02 is 258 and the body is absent, so the answer is wait (1), not invalid");
 uint8_t bad_then_data[]={25,3,3,0,5};
 CHECK("invalid header is judged with the body missing",tls_walk(bad_then_data,5,&rr,&cc)==-1,"a 5-byte header with type 25 is invalid; do not wait for a body");
 uint8_t hs[]={22,3,3,0,0};size_t r1=9,c1=9;
 CHECK("zero length is invalid",tls_walk(hs,5,&r1,&c1)==-1&&r1==0&&c1==0,"");
 uint8_t big[]={23,3,3,0x40,0x01};
 CHECK("length above 16384",tls_walk(big,5,NULL,NULL)==-1,"");
 uint8_t part[]={23,3,3,0x40,0x00,1,2,3};
 CHECK("header is judged before the body arrives",tls_walk(part,8,&r1,&c1)==1&&r1==0&&c1==0,"");
 CHECK("partial header",tls_walk(part,3,&r1,&c1)==1,"");
 CHECK("NULL buffer",tls_walk(NULL,4,&r1,&c1)==-1&&r1==0&&c1==0&&tls_walk(NULL,0,&r1,&c1)==0,"");
 return 0;}
