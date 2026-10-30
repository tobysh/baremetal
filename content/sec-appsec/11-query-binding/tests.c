static uint32_t rs=777u;
static uint32_t rnd(void){ rs^=rs<<13; rs^=rs>>17; rs^=rs<<5; return rs; }
#define GP 64
static int run(const char *tmpl,const char *const *a,int na,size_t cap,char *res,size_t rcap,int *intact){
  uint8_t mem[GP+256+GP]; memset(mem,0xA5,sizeof mem); char *out=(char*)mem+GP; if(cap>256)cap=256;
  int rc=bind_query(tmpl,a,na,out,cap);
  *intact=1; for(int i=0;i<GP;i++) if(mem[i]!=0xA5)*intact=0; for(size_t i=cap;i<256+GP;i++) if(mem[GP+i]!=0xA5)*intact=0;
  if(rc>=0&&res){ snprintf(res,rcap,"%s",out); }
  if(rc<0&&cap>0&&out[0]!=0)*intact=0;
  return rc;
}
int main(void){
  char res[400]; int ok; const char *a1[1]; const char *a2[2];
  a1[0]="alice"; int rc=run("SELECT * FROM u WHERE name = ?",a1,1,256,res,sizeof res,&ok);
  CHECK("simple binding", rc==(int)strlen("SELECT * FROM u WHERE name = 'alice'") && !strcmp(res,"SELECT * FROM u WHERE name = 'alice'") && ok, "rc=%d out=%s",rc,res);
  a1[0]="x' OR '1'='1"; rc=run("SELECT * FROM u WHERE name = ?",a1,1,256,res,sizeof res,&ok);
  CHECK("quotes in values are doubled", !strcmp(res,"SELECT * FROM u WHERE name = 'x'' OR ''1''=''1'") && ok, "out=%s",res);
  a1[0]="'; DROP TABLE u; --"; rc=run("DELETE FROM t WHERE id = ?",a1,1,256,res,sizeof res,&ok);
  CHECK("injection text stays a value", !strcmp(res,"DELETE FROM t WHERE id = '''; DROP TABLE u; --'") && ok, "out=%s",res);
  a2[0]="a"; a2[1]="b"; rc=run("SELECT ? , '?' , ?",a2,2,256,res,sizeof res,&ok);
  CHECK("question mark inside a literal is not a placeholder", !strcmp(res,"SELECT 'a' , '?' , 'b'") && ok, "out=%s",res);
  a1[0]="x' OR '1'='1"; rc=run("SELECT 'it''s ?' , ?",a1,1,256,res,sizeof res,&ok);
  CHECK("doubled quote inside a literal keeps the literal open", rc>0 && !strcmp(res,"SELECT 'it''s ?' , 'x'' OR ''1''=''1'") && ok, "rc=%d out=%s",rc,res);
  rc=run("SELECT ?",a2,2,256,res,sizeof res,&ok);
  CHECK("too many arguments is an error", rc==-1 && ok, "rc=%d",rc);
  rc=run("SELECT ?, ?",a1,1,256,res,sizeof res,&ok);
  CHECK("too few arguments is an error", rc==-1 && ok, "rc=%d",rc);
  rc=run("SELECT 'open ?",a1,1,256,res,sizeof res,&ok);
  CHECK("unterminated literal is an error", rc==-1 && ok, "rc=%d",rc);
  a1[0]="line\nbreak"; rc=run("SELECT ?",a1,1,256,res,sizeof res,&ok);
  CHECK("control characters in values are rejected", rc==-1 && ok, "rc=%d",rc);
  rc=run("SELECT 1",NULL,0,256,res,sizeof res,&ok);
  CHECK("no placeholders needs no arguments", rc==8 && !strcmp(res,"SELECT 1") && ok, "rc=%d",rc);
  a1[0]=""; rc=run("SELECT ?",a1,1,256,res,sizeof res,&ok);
  CHECK("empty value becomes an empty literal", rc==9 && !strcmp(res,"SELECT ''") && ok, "rc=%d out=%s",rc,res);
  a1[0]="abc"; { size_t need=strlen("SELECT 'abc'"); 
    rc=run("SELECT ?",a1,1,need+1,res,sizeof res,&ok); CHECK("exact fit succeeds", rc==(int)need && ok, "rc=%d",rc);
    rc=run("SELECT ?",a1,1,need,res,sizeof res,&ok); CHECK("one byte short fails cleanly", rc==-1 && ok, "rc=%d",rc);
    rc=run("SELECT ?",a1,1,0,res,sizeof res,&ok); CHECK("zero capacity fails cleanly", rc==-1 && ok, "rc=%d",rc); }
  for(int t=0;t<25;t++){
    char v[60]; size_t l=rnd()%40; size_t q=0; for(size_t i=0;i<l;i++){ v[i]=(rnd()%4==0)?'\'':(char)('a'+rnd()%26); if(v[i]=='\'')q++; } v[l]=0;
    char want[200]; snprintf(want,sizeof want,"WHERE x = '"); size_t k=strlen(want); for(size_t i=0;i<l;i++){ want[k++]=v[i]; if(v[i]=='\'')want[k++]='\''; } want[k++]='\''; want[k]=0;
    a1[0]=v; size_t cap=(rnd()%3==0)?k:k+1+rnd()%5;
    rc=run("WHERE x = ?",a1,1,cap,res,sizeof res,&ok);
    int exp_ok=cap>=k+1;
    CHECK("random value", ok && (exp_ok ? (rc==(int)k && !strcmp(res,want)) : rc==-1), "v=%s cap=%zu rc=%d",v,cap,rc);
    (void)q;
  }
  return 0;
}
