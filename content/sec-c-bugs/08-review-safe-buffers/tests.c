int main(void){uint8_t src[12]={'%',0,'n',255,4,5,6,7,8,9,10,11};
 for(int len=-2;len<=12;len++)for(size_t cap=0;cap<=12;cap++){uint8_t out[32];memset(out,0xa5,sizeof out);int good=len>=0 && (size_t)len<cap;int rc=record_copy(out+3,cap,src,len),ok=rc==good;
 for(size_t i=0;i<32;i++){uint8_t w=0xa5;if(good&&i>=3&&i<3+(size_t)len)w=src[i-3];if(good&&i==3+(size_t)len)w=0;if(out[i]!=w)ok=0;}
 CHECK("record boundaries",ok,"len %d cap %zu",len,cap);}
 uint8_t d=9;CHECK("NULL source",record_copy(&d,1,NULL,0)==1 && d==0 && record_copy(&d,2,NULL,1)==0,"");
 CHECK("NULL dst",record_copy(NULL,2,src,1)==0,"");return 0;}
