int main(void){
 const char*users[]={"al","bob","carolyn"};const char*hosts[]={"h","example.test","a-very-long-host.example.test"};
 for(int u=0;u<3;u++)for(int h=0;h<3;h++)for(size_t cap=0;cap<=40;cap++){
  uint8_t raw[8+40+8];memset(raw,0xA5,sizeof raw);char*out=(char*)raw+8;
  size_t full=strlen(users[u])+1+strlen(hosts[h]);int fits=full<cap;
  int rc=build_label(out,cap,users[u],hosts[h]);
  int guards=1;for(size_t i=0;i<8;i++)if(raw[i]!=0xA5||raw[8+cap+i]!=0xA5)guards=0;
  char want[100];snprintf(want,sizeof want,"%s@%s",users[u],hosts[h]);
  int body=fits?!strcmp(out,want):(cap==0?raw[8]==0xA5:out[0]==0);
  CHECK("capacity sweep",rc==fits&&guards&&body,"user %d host %d cap %zu: rc %d",u,h,cap,rc);}
 char o[16]="zz";
 CHECK("NULL and empty names",build_label(o,16,NULL,"h")==0&&o[0]==0&&build_label(o,16,"u","")==0&&build_label(o,16,"","h")==0&&build_label(NULL,16,"u","h")==0,"");
 return 0;}
