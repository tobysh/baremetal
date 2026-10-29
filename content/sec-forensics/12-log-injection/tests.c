static size_t piece(unsigned char c,char*p){static const char hx[]="0123456789abcdef";
 if(c=='\n'){memcpy(p,"\\n",2);return 2;}if(c=='\r'){memcpy(p,"\\r",2);return 2;}if(c=='\\'){memcpy(p,"\\\\",2);return 2;}
 if(c<0x20||c==0x7f){p[0]='\\';p[1]='x';p[2]=hx[c>>4];p[3]=hx[c&15];return 4;}p[0]=(char)c;return 1;}
int main(void){
 const char*in[]={"plain","a\nb","x\r\ny\\z","\x01\x7f","","tab\there","user=\"bob\"\nINFO login ok","caf\xc3\xa9\x80","\\n literal","\x1f\x20\x7e"};
 for(size_t k=0;k<sizeof in/sizeof in[0];k++){
  char full[400];size_t total=0;for(const unsigned char*p=(const unsigned char*)in[k];*p;p++)total+=piece(*p,full+total);full[total]=0;
  CHECK("return value covers the whole text",log_escape(NULL,0,in[k])==total,"input %zu",k);
  for(size_t cap=0;cap<=total+2;cap++){
   uint8_t raw[8+400+8];memset(raw,0xA5,sizeof raw);
   size_t rc=log_escape((char*)raw+8,cap,in[k]);
   int guards=1;for(size_t i=0;i<8;i++)if(raw[i]!=0xA5||raw[8+cap+i]!=0xA5)guards=0;
   char want[400];size_t w=0;int stop=0;for(const unsigned char*p=(const unsigned char*)in[k];*p;p++){char pc[4];size_t l=piece(*p,pc);
    if(!stop&&cap&&w+l<cap){memcpy(want+w,pc,l);w+=l;}else stop=1;}
   int body=cap==0?raw[8]==0xA5:(raw[8+w]==0&&!memcmp(raw+8,want,w));
   CHECK("capacity sweep",rc==total&&guards&&body,"input %zu cap %zu: returned %zu (want %zu)",k,cap,rc,total);}}
 char o[8]="zzzzzzz";
 CHECK("NULL input",log_escape(o,8,NULL)==0&&o[0]==0,"");
 CHECK("NULL output",log_escape(NULL,10,"a\nb")==4,"");
 return 0;}
