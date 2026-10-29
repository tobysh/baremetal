static uint32_t st=4242;static uint32_t rnd(void){st=st*1664525u+1013904223u;return st>>8;}
int main(void){
 static const struct{const char*s;int ok;}M[]={{"GET",1},{"HEAD",1},{"POST",1},{"get",0},{"PUT",0},{"",0},{"GETS",0},{"GE",0}};
 static const struct{const char*s;int ok;}P[]={{"/",1},{"/a/b",1},{"/a/../b",0},{"/..",0},{"/a/..b/c",1},{"a/b",0},{"/q?x=1&y=../",1},{"/t\tab",0},{"",0},{"/a/..",0},{"/.../x",1},{"/x\x7f",0},{"//..//",0},{"/caf\xc3\xa9",0}};
 static const struct{const char*s;int ok;}V[]={{"HTTP/1.1",1},{"HTTP/1.0",1},{"HTTP/2",0},{"HTTP/1.1\r",0},{"http/1.1",0},{"HTTP/1.1 x",0},{"",0}};
 for(int it=0;it<600;it++){
  int m=rnd()%8,p=rnd()%14,v=rnd()%7,sp1=rnd()%5==0,sp2=rnd()%5==0;
  char line[200];snprintf(line,sizeof line,"%s%s%s%s%s",M[m].s,sp1?"  ":" ",P[p].s,sp2?"  ":" ",V[v].s);
  int want=M[m].ok&&P[p].ok&&V[v].ok&&!sp1&&!sp2;
  char mo[16]="x",po[64]="y";int rc=parse_request(line,mo,sizeof mo,po,sizeof po);
  int ok=rc==want&&(want?(!strcmp(mo,M[m].s)&&!strcmp(po,P[p].s)):(mo[0]==0&&po[0]==0));
  CHECK("generated lines",ok,"line \"%s\": rc %d, want %d",line,rc,want);}
 {char mo[8],po[32];
  CHECK("exact fit",parse_request("POST /abcd HTTP/1.1",mo,5,po,6)==1&&!strcmp(mo,"POST")&&!strcmp(po,"/abcd"),"");
  CHECK("method too small",parse_request("POST /abcd HTTP/1.1",mo,4,po,6)==0&&mo[0]==0&&po[0]==0,"");
  CHECK("path too small",parse_request("POST /abcd HTTP/1.1",mo,5,po,5)==0&&mo[0]==0&&po[0]==0,"");
  CHECK("NULL arguments",parse_request(NULL,mo,8,po,32)==0&&parse_request("GET / HTTP/1.1",NULL,8,po,32)==0&&parse_request("GET / HTTP/1.1",mo,8,NULL,32)==0&&parse_request("GET / HTTP/1.1",mo,0,po,32)==0,"");}
 return 0;}
