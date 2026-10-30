static uint32_t st=99;static uint32_t rnd(void){st=st*1664525u+1013904223u;return st>>8;}
static uint64_t ref(const char*pw,const uint8_t*salt,size_t sl,unsigned it){
 uint8_t buf[512];size_t n=0;memcpy(buf,salt?salt:(const uint8_t*)"",sl);n=sl;size_t pl=pw?strlen(pw):0;memcpy(buf+n,pw?pw:"",pl);n+=pl;
 uint64_t h=14695981039346656037ULL;for(size_t i=0;i<n;i++){h^=buf[i];h*=1099511628211ULL;}
 for(unsigned k=0;k<(it?it:1);k++){uint8_t t[8+64];for(int j=0;j<8;j++)t[j]=(uint8_t)(h>>(8*j));memcpy(t+8,salt?salt:(const uint8_t*)"",sl);
  h=14695981039346656037ULL;for(size_t i=0;i<8+sl;i++){h^=t[i];h*=1099511628211ULL;}}
 return h;}
int main(void){
 static const struct{const char*pw;const char*salt;size_t sl;unsigned it;uint64_t want;}V[]={{"\x68\x75\x6e\x74\x65\x72\x32","\x01\x02\x03\x04",4,1,0xf4f68383f2f30ac1ULL},{"\x68\x75\x6e\x74\x65\x72\x32","\x01\x02\x03\x05",4,1,0x527fb8a7e6bb4116ULL},{"\x68\x75\x6e\x74\x65\x72\x32","\x01\x02\x03\x04",4,1000,0x38f9e35f105480d8ULL},{"","",0,5,0x24ac8ba692176d59ULL},{"\x63\x6f\x72\x72\x65\x63\x74\x20\x68\x6f\x72\x73\x65","\x73\x61\x6c\x74\x79",5,250,0xf4d30a52445c0b7aULL},{"\x78","\xff\x00\xff",3,3,0xf63987c4d027fbdbULL}};
 for(size_t i=0;i<sizeof V/sizeof V[0];i++){uint64_t g=toy_kdf(V[i].pw,(const uint8_t*)V[i].salt,V[i].sl,V[i].it);
  CHECK("fixed vectors",g==V[i].want,"vector %zu: got %016llx want %016llx",i,(unsigned long long)g,(unsigned long long)V[i].want);}
 for(int t=0;t<300;t++){char pw[40];size_t pl=rnd()%20;for(size_t i=0;i<pl;i++)pw[i]=(char)(1+rnd()%126);pw[pl]=0;
  uint8_t s[16];size_t sl=rnd()%17;for(size_t i=0;i<sl;i++)s[i]=(uint8_t)rnd();unsigned it=rnd()%40;
  uint64_t g=toy_kdf(pw,s,sl,it),w=ref(pw,s,sl,it);
  CHECK("generated inputs",g==w,"password length %zu, salt length %zu, iterations %u",pl,sl,it);}
 uint8_t s1[4]={1,2,3,4},s2[4]={1,2,3,5};
 CHECK("salt matters",toy_kdf("pw",s1,4,10)!=toy_kdf("pw",s2,4,10),"");
 CHECK("iterations matter",toy_kdf("pw",s1,4,10)!=toy_kdf("pw",s1,4,11),"");
 CHECK("zero iterations is one",toy_kdf("pw",s1,4,0)==toy_kdf("pw",s1,4,1),"");
 CHECK("NULL inputs",toy_kdf(NULL,NULL,0,2)==toy_kdf("",(const uint8_t*)"",0,2),"");
 return 0;}
