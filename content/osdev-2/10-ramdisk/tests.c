void kmain(void) { uint8_t a[]={4,8,15,16,23,42}, b[6]={0}; disk_init(); CHECK("write and read",disk_write(123,a,6)==0 && disk_read(123,b,6)==0 && b[0]==4 && b[5]==42,"data mismatch");
 CHECK("end and empty",disk_write(512,a,0)==0 && disk_read(512,b,0)==0 && disk_write(511,a,1)==0,"boundary failed");
 CHECK("invalid ranges",disk_write(510,a,6)==-1 && disk_read((size_t)-2,b,6)==-1 && disk_write(513,a,0)==-1,"invalid range accepted");
 CHECK("failure is atomic",disk_read(511,b,1)==0 && b[0]==4,"failed write changed disk"); disk_init(); CHECK("reset zeroes",disk_read(123,b,6)==0 && b[0]==0 && b[5]==0,"not reset"); }
