void kmain(void) { Pipe p; uint8_t a[24],b[24]; for(int i=0;i<24;i++) a[i]=i+1; pipe_init(&p);
 CHECK("empty is not EOF",pipe_read(&p,b,1)==-1 && pipe_read(&p,b,0)==0,"empty result"); CHECK("bounded write",pipe_write(&p,a,20)==16 && pipe_write(&p,a,1)==0,"capacity failed");
 int ok=pipe_read(&p,b,10)==10; for(int i=0;i<10;i++) if(b[i]!=a[i]) ok=0; CHECK("first read",ok,"data mismatch");
 CHECK("wrap write",pipe_write(&p,a+16,8)==8,"write failed"); pipe_close_writer(&p); int n=pipe_read(&p,b,24); ok=n==14; for(int i=0;i<14 && ok;i++) if(b[i]!=a[i+10]) ok=0;
 CHECK("drain after close",ok,"wrong bytes"); CHECK("EOF and closed write",pipe_read(&p,b,1)==0 && pipe_write(&p,a,0)==-1,"close failed"); }
