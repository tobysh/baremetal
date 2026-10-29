static const char *AL="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
static size_t enc(const uint8_t *b,size_t n,char *o){ size_t k=0; uint32_t acc=0; int bits=0; for(size_t i=0;i<n;i++){ acc=(acc<<8)|b[i]; bits+=8; while(bits>=6){ bits-=6; o[k++]=AL[(acc>>bits)&63]; } acc&=(1u<<bits)-1u; } if(bits)o[k++]=AL[(acc<<(6-bits))&63]; o[k]=0; return k; }
static uint32_t rs=2463534242u;
static uint32_t rnd(void){ rs^=rs<<13; rs^=rs>>17; rs^=rs<<5; return rs; }
int main(void){
  uint8_t in[200],out[300]; char s[400]; size_t len;
  for(int t=0;t<40;t++){
    size_t n=(size_t)(rnd()%90); for(size_t i=0;i<n;i++) in[i]=(uint8_t)rnd();
    size_t sl=enc(in,n,s); len=12345;
    int rc=b64u_decode(s,sl,out,sizeof out,&len);
    CHECK("round trip", rc==0 && len==n && memcmp(in,out,n)==0, "n=%zu rc=%d len=%zu", n, rc, len);
  }
  len=7; CHECK("empty input decodes to nothing", b64u_decode("",0,out,sizeof out,&len)==0 && len==0, "");
  CHECK("known value", b64u_decode("aGVsbG8",7,out,sizeof out,&len)==0 && len==5 && memcmp(out,"hello",5)==0, "");
  CHECK("url alphabet is accepted", b64u_decode("-_-_",4,out,sizeof out,&len)==0 && len==3 && out[0]==0xfb && out[1]==0xff && out[2]==0xbf, "");
  CHECK("plus is rejected", b64u_decode("ab+d",4,out,sizeof out,&len)==-1, "");
  CHECK("slash is rejected", b64u_decode("ab/d",4,out,sizeof out,&len)==-1, "");
  CHECK("padding is rejected", b64u_decode("aGk=",4,out,sizeof out,&len)==-1, "");
  CHECK("space is rejected", b64u_decode("aG k",4,out,sizeof out,&len)==-1, "");
  CHECK("newline is rejected", b64u_decode("aGk\n",4,out,sizeof out,&len)==-1, "");
  CHECK("length 1 mod 4 is impossible", b64u_decode("a",1,out,sizeof out,&len)==-1 && b64u_decode("abcde",5,out,sizeof out,&len)==-1, "");
  CHECK("non-canonical trailing bits are rejected", b64u_decode("QR",2,out,sizeof out,&len)==-1 && b64u_decode("QQ",2,out,sizeof out,&len)==0 && out[0]==0x41, "");
  CHECK("non-canonical with three characters", b64u_decode("QUF",3,out,sizeof out,&len)==-1 && b64u_decode("QUE",3,out,sizeof out,&len)==0, "");
  CHECK("embedded NUL byte is rejected", b64u_decode("ab\0d",4,out,sizeof out,&len)==-1, "");
  for(int t=0;t<20;t++){
    size_t n=3+(size_t)(rnd()%40); for(size_t i=0;i<n;i++) in[i]=(uint8_t)rnd();
    size_t sl=enc(in,n,s); size_t cap=n-1-(size_t)(rnd()%2) ; if(cap>n)cap=0;
    uint8_t guard[64]; memset(guard,0xA5,sizeof guard); uint8_t *small=guard+16;
    int rc=b64u_decode(s,sl,small,cap,&len);
    int intact=1; for(size_t i=cap;i<48;i++) if(small[i]!=0xA5)intact=0; for(int i=0;i<16;i++) if(guard[i]!=0xA5)intact=0;
    CHECK("output buffer too small is an error and nothing is written past it", rc==-1 && intact, "n=%zu cap=%zu rc=%d intact=%d", n, cap, rc, intact);
  }
  return 0;
}
