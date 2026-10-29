int main(void){uint8_t s[10]={0,1,2,3,4,5,6,7,8,9};int lens[]={INT_MIN,-20,-1,0,1,4,8,10};
 for(size_t c=0;c<=8;c++)for(size_t j=0;j<8;j++){uint8_t d[32];memset(d,0x55,sizeof d);int n=lens[j],good=n>=0 && (size_t)n<=c;int rc=copy_signed(d+4,c,s,n),ok=rc==(good?n:-1);
 for(size_t k=0;k<32;k++)if(d[k]!=(good&&k>=4&&k<4+(size_t)n?s[k-4]:0x55))ok=0;
 CHECK("signed boundary",ok,"cap=%zu length=%d",c,n);}
 CHECK("zero allows NULL",copy_signed(NULL,0,NULL,0)==0,"");
 CHECK("positive requires pointers",copy_signed(NULL,4,s,1)==-1 && copy_signed(s,4,NULL,1)==-1,"");return 0;}
