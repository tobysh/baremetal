static uint32_t st=777;static uint32_t rnd(void){st=st*1664525u+1013904223u;return st>>8;}
static int ref(const char*const*segs,int n,char*out){const char*stk[16];int d=0;
 for(int i=0;i<n;i++){const char*s=segs[i];if(!*s||!strcmp(s,"."))continue;
  if(!strcmp(s,"..")){if(!d)return 0;d--;continue;}stk[d++]=s;}
 out[0]=0;if(!d){strcpy(out,"/");return 1;}
 for(int i=0;i<d;i++){strcat(out,"/");strcat(out,stk[i]);}return 1;}
static int guards_ok(const uint8_t*raw,size_t cap){for(size_t i=0;i<8;i++)if(raw[i]!=0xA5||raw[8+cap+i]!=0xA5)return 0;return 1;}
static int run(const char*in,size_t cap,char*res,int*rc){uint8_t raw[8+128+8];memset(raw,0xA5,sizeof raw);
 *rc=clean_path(in,(char*)raw+8,cap);memcpy(res,raw+8,cap?cap:1);return guards_ok(raw,cap);}
int main(void){
 static const struct{const char*in;const char*want;}T[]={
  {"/","/"},{"","-"},{"a","-"},{"//","/"},{"/a/b/c","/a/b/c"},{"/a//b/./c/","/a/b/c"},{"/a/b/../c","/a/c"},{"/a/..","/"},
  {"/..","-"},{"/a/../..","-"},{"/a/b/../../c","/c"},{"/...","/..."},{"/..a/b..","/..a/b.."},{"/a\\b","-"},{"/a\nb","-"},
  {"/a/./../b","/b"},{"/./.","/"},{"/x/y/z/../../../w","/w"},{"/x/y/z/../../../../w","-"},{"/etc/../etc/passwd","/etc/passwd"},
  {"/a/b/","/a/b"},{"/a/\x7f","-"},{"/a/\x01","-"},{"/caf\xc3\xa9/../x","/x"}};
 for(size_t i=0;i<sizeof T/sizeof T[0];i++){
  char res[128];int rc;int g=run(T[i].in,64,res,&rc);int ok=T[i].want[0]=='-'?(rc==0&&res[0]==0):(rc==1&&!strcmp(res,T[i].want));
  CHECK("fixed paths",ok&&g,"input #%zu: rc %d, got \"%s\"",i,rc,res);
  if(T[i].want[0]!='-'){size_t need=strlen(T[i].want)+1;
   for(size_t cap=0;cap<=need+1;cap++){int rc2;char r2[128];int g2=run(T[i].in,cap,r2,&rc2);
    int want=cap>=need;int ok2=rc2==want&&g2&&(want?!strcmp(r2,T[i].want):(cap==0||r2[0]==0));
    CHECK("capacity boundary",ok2,"input #%zu cap %zu: rc %d",i,cap,rc2);}}}
 static const char*pool[]={"a","bb","ccc",".","..","","d.e"};
 for(int it=0;it<400;it++){
  const char*segs[10];int n=1+rnd()%8;char in[160]="";
  for(int i=0;i<n;i++){segs[i]=pool[rnd()%7];strcat(in,"/");strcat(in,segs[i]);}
  if(rnd()%3==0)strcat(in,"/");
  char want[160];int wr=ref(segs,n,want);char res[128];int rc;int g=run(in,64,res,&rc);
  CHECK("generated paths",g&&rc==wr&&(wr?!strcmp(res,want):res[0]==0),"input \"%s\": rc %d got \"%s\", want %d \"%s\"",in,rc,res,wr,want);}
 char o[4];CHECK("NULL arguments",clean_path(NULL,o,4)==0&&clean_path("/a",NULL,4)==0&&clean_path("/a",o,0)==0,"");
 return 0;}
