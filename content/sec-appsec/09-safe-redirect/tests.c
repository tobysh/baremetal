static uint32_t rs=123456789u;
static uint32_t rnd(void){ rs^=rs<<13; rs^=rs>>17; rs^=rs<<5; return rs; }
static const char *SAFE[]={"/","/dashboard","/a/b/c","/search?q=hello&page=2","/docs#section-3","/files/report%20final.txt","/a//b","/~user/x","/a.b-c_d","/%41bc","/x?next=/y"};
static const char *BAD[]={"","dashboard","//evil.test","///","/\\evil.test","/%2fevil.test","/%2Fevil.test","/%5cevil.test","/a\\b","/a b","/a\tb","/a\nb","/a\rb","/%0d%0aSet-Cookie:x","/%0A","/%00","/%7f","/%zz","/%4","/%","http://evil.test","https://evil.test/","javascript:alert(1)","data:text/html,x","\\\\evil.test"," /x","/x ","/\x7f"};
int main(void){
  char buf[4200];
  for(size_t i=0;i<sizeof SAFE/sizeof *SAFE;i++){ char n[64]; snprintf(n,sizeof n,"safe path %zu",i); CHECK(n, safe_redirect(SAFE[i])==1, "%s should be accepted",SAFE[i]); }
  for(size_t i=0;i<sizeof BAD/sizeof *BAD;i++){ char n[64]; snprintf(n,sizeof n,"hostile target %zu",i); CHECK(n, safe_redirect(BAD[i])==0, "%s should be rejected",BAD[i]); }
  CHECK("NULL is rejected", safe_redirect(NULL)==0, "");
  buf[0]='/'; memset(buf+1,'a',2047); buf[2048]=0;
  CHECK("2048 characters is allowed", safe_redirect(buf)==1, "");
  buf[0]='/'; memset(buf+1,'a',2048); buf[2049]=0;
  CHECK("2049 characters is too long", safe_redirect(buf)==0, "");
  for(int t=0;t<30;t++){
    int len=1+(int)(rnd()%20); buf[0]='/'; int k=1;
    for(int i=0;i<len;i++){ const char *al="abcxyz019-_.~/?=&#"; char c=al[rnd()%18]; if(i==0&&c=='/')c='a'; buf[k++]=c; }
    buf[k]=0; CHECK("generated safe path", safe_redirect(buf)==1, "%s",buf);
  }
  const char *pre[]={"/","/x/","/x?y="}; const char *ev[]={"\\","%5c","%5C","%0a","%0D","%09","\t"," ","%1f","\x01"};
  for(int t=0;t<30;t++){
    const char *p=pre[rnd()%3],*e=ev[rnd()%10]; snprintf(buf,sizeof buf,"%s%s%s",p,e,"end");
    CHECK("generated hostile path", safe_redirect(buf)==0, "%s",buf);
  }
  for(int t=0;t<12;t++){ const char *e[]={"//","/\\","/%2f","/%2F","/%5c","/%5C"}; snprintf(buf,sizeof buf,"%s%s.test/path",e[t%6],"evil");
    CHECK("scheme-relative host forms", safe_redirect(buf)==0, "%s",buf); }
  return 0;
}
