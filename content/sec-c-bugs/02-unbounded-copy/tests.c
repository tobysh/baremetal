int main(void){
 uint8_t s[12]={1,0,3,4,5,6,7,8,9,10,11,12};
 for(size_t cap=0;cap<=8;cap++) for(size_t n=0;n<=12;n++){
  uint8_t d[32]; memset(d,0xa5,sizeof d);
  int rc=copy_bytes(d+4,cap,s,n); int ok=rc==(n<=cap);
  for(size_t i=0;i<32;i++) {uint8_t want=(n<=cap && i>=4 && i<4+n)?s[i-4]:0xa5; if(d[i]!=want)ok=0;}
  CHECK("length and guard bytes",ok,"cap=%zu n=%zu",cap,n);
 }
 CHECK("zero bytes allow NULL",copy_bytes(NULL,0,NULL,0)==1,"");
 uint8_t d=42;
 CHECK("NULL source rejected",copy_bytes(&d,1,NULL,1)==0 && d==42,"");
 CHECK("NULL destination rejected",copy_bytes(NULL,1,s,1)==0,""); return 0;
}
