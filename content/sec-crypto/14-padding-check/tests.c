static uint32_t st=31337;static uint32_t rnd(void){st=st*1664525u+1013904223u;return st>>8;}
int main(void){
 size_t sizes[]={1,8,16,255};
 for(int it=0;it<600;it++){
  size_t bs=sizes[rnd()%4];size_t blocks=1+rnd()%3;size_t n=blocks*bs;uint8_t b[255*3];
  for(size_t i=0;i<n;i++)b[i]=(uint8_t)rnd();
  size_t p=1+rnd()%bs;for(size_t i=0;i<p;i++)b[n-1-i]=(uint8_t)p;
  int want=1,kind=rnd()%6;
  if(kind==1&&p>1){b[n-1-rnd()%(p-1)-1]^=(uint8_t)(1+rnd()%255);want=0;}
  if(kind==2){b[n-1]=0;want=0;}
  if(kind==3&&bs<255){b[n-1]=(uint8_t)(bs+1+rnd()%(255-bs));want=0;}
  size_t useN=n;if(kind==4&&bs>1){useN=n-1;want=-1;}
  if(kind==5){b[n-1]=(uint8_t)(p=1);want=1;}
  size_t dl=77,wk=77;int rc=pkcs7_unpad(b,useN,bs,&dl,&wk);
  if(want==-1)CHECK("odd length",rc==0&&dl==0&&wk==0,"bs %zu n %zu",bs,useN);
  else CHECK("padding",rc==want&&dl==(want?n-(kind==5?1:p):0)&&wk==bs,"bs %zu kind %d: rc %d datalen %zu work %zu",bs,kind,rc,dl,wk);}
 uint8_t z[16]={0};size_t dl=5,wk=5;
 CHECK("shape errors",pkcs7_unpad(NULL,16,16,&dl,&wk)==0&&pkcs7_unpad(z,0,16,&dl,&wk)==0&&pkcs7_unpad(z,16,0,&dl,&wk)==0&&pkcs7_unpad(z,16,256,&dl,&wk)==0&&dl==0&&wk==0,"");
 uint8_t ok[8]={1,2,3,4,5,3,3,3};
 CHECK("optional outputs",pkcs7_unpad(ok,8,8,NULL,NULL)==1,"");
 return 0;}
