int main(void){uint8_t a[64],b[64];for(size_t i=0;i<64;i++)a[i]=(uint8_t)(i*17);
 for(size_t n=1;n<=64;n*=2){memcpy(b,a,n);size_t v=99;CHECK("equal full scan",equal_bytes(a,b,n,&v)==1 && v==n,"");
 for(size_t k=0;k<n;k++){memcpy(b,a,n);b[k]^=0x80;int rc=equal_bytes(a,b,n,&v);CHECK("mismatch full scan",rc==0 && v==n,"n=%zu position=%zu visited=%zu",n,k,v);}}
 size_t v=1;CHECK("empty",equal_bytes(NULL,NULL,0,&v)==1 && v==0,"");
 CHECK("invalid",equal_bytes(NULL,a,3,&v)==0 && v==0,"");CHECK("optional count",equal_bytes(a,a,64,NULL)==1,"");return 0;}
