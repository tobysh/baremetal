static const uint8_t request[]={71,69,84,32,47,110,111,116,101,115,32,72,84,84,80,47,49,46,49,13,10,72,111,115,116,58,32,101,120,97,109,112,108,101,46,116,101,115,116,13,10,13,10};

int main(void){const size_t cuts[]={1,4,12,20};for(size_t i=0;i<4;i++){size_t k=cuts[i];Segment s[]={{1000+(uint32_t)k,request+k,sizeof request-k},{1000,request,k},{1000,request,k}};char out[32];int rc=http_path(s,3,1000,out,sizeof out);CHECK("out-of-order and duplicate",rc==6&&!strcmp(out,"/notes"),"cut %zu",k);}
 const uint8_t other[]="GET /a/b?q=7 HTTP/1.1\r\n\r\n";Segment s={9,other,sizeof other-1};char out[32];CHECK("another path",http_path(&s,1,9,out,sizeof out)==8&&!strcmp(out,"/a/b?q=7"),"");
 Segment gap[]={{1001,request+1,sizeof request-1}};CHECK("missing first byte",http_path(gap,1,1000,out,sizeof out)==-1,"");
 const uint8_t bad[]="X";Segment conflict[]={{1000,request,sizeof request},{1000,bad,1}};CHECK("conflicting overlap",http_path(conflict,2,1000,out,sizeof out)==-1,"");
 Segment whole={1000,request,sizeof request};CHECK("small output",http_path(&whole,1,1000,out,6)==-1,"");
 Segment partial={1000,request,20};CHECK("incomplete headers",http_path(&partial,1,1000,out,sizeof out)==-1,"");
 const uint8_t wrong[]="POST / HTTP/1.1\r\n\r\n";Segment post={1,wrong,sizeof wrong-1};CHECK("unsupported method",http_path(&post,1,1,out,sizeof out)==-1,"");
 CHECK("empty stream",http_path(NULL,0,1000,out,sizeof out)==-1,"");return 0;}
