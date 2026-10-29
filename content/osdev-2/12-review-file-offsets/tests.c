void kmain(void) { uint8_t a[40],b[40]; files_init(); int x=file_open(),y=file_open(); CHECK("open slots",x==0 && y==1 && file_open()==2 && file_open()==3 && file_open()==-1,"wrong slots");
 CHECK("read advances offset",file_read(x,a,5)==5 && a[0]==0 && a[4]==4 && file_read(x,a,3)==3 && a[0]==5,"wrong offset");
 CHECK("independent offset",file_read(y,b,3)==3 && b[0]==0 && b[2]==2,"shared offset");
 CHECK("short read and EOF",file_read(x,a,40)==24 && a[0]==8 && a[23]==31 && file_read(x,a,1)==0,"EOF failed");
 CHECK("invalid descriptors",file_read(-1,a,1)==-1 && file_close(4)==-1 && file_close(y)==0 && file_read(y,a,0)==-1 && file_close(y)==-1,"invalid accepted");
 CHECK("reuse starts at zero",file_open()==1 && file_read(1,b,2)==2 && b[0]==0 && b[1]==1,"reuse failed"); }
