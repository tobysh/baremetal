int main(void){uint8_t e[]={0,0xab,255,0x10};
 const char *s[]={"00abff10","00ABFF10","00abff11","01abff10","00abff1","00abff1000","00agff10","00 abff10","00abff1x"};
 for(size_t i=0;i<9;i++)CHECK("strict token bytes",token_equal(s[i],e,4)==(i<2),"case %zu",i);
 CHECK("empty",token_equal("",NULL,0)==1,"");CHECK("NULL",token_equal(NULL,e,4)==0 && token_equal("aa",NULL,1)==0,"");
 for(unsigned v=0;v<256;v++){char h[3];uint8_t b=(uint8_t)v;snprintf(h,sizeof h,"%02x",v);CHECK("all bytes",token_equal(h,&b,1)==1,"byte %u",v);}return 0;}
