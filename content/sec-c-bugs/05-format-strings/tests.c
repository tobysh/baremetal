int main(void){const char *s[]={"plain","%s %x %n","100% complete","%%","","line\nnext",NULL};
 for(size_t j=0;j<7;j++)for(size_t cap=0;cap<=14;cap++){char d[32];memset(d,'Z',sizeof d);const char *p=s[j]?s[j]:"";size_t n=strlen(p),k=cap?(n<cap-1?n:cap-1):0;size_t rc=literal_message(d+2,cap,s[j]);
 int ok=rc==n && (!cap || (memcmp(d+2,p,k)==0 && d[2+k]==0));for(size_t x=0;x<32;x++)if((x<2 || x>=2+cap)&&d[x]!='Z')ok=0;
 CHECK("literal formatting and bounds",ok,"text %zu cap %zu",j,cap);}
 CHECK("sizing call",literal_message(NULL,0,"%n")==2,"");return 0;}
