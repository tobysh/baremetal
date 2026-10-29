int main(void){uint8_t x[96];for(size_t b=1;b<=24;b++){for(size_t i=0;i<96;i++)x[i]=(uint8_t)i;
 CHECK("distinct blocks",repeated_block(x,b*4,b)==0,"block size %zu",b);
 memcpy(x+3*b,x,b);CHECK("nonadjacent repeat",repeated_block(x,b*4,b)==1,"block size %zu",b);
 CHECK("partial tail ignored",repeated_block(x,b*4-1,b)==0,"block size %zu",b);}
 CHECK("invalid or short",repeated_block(NULL,100,16)==0 && repeated_block(x,96,0)==0 && repeated_block(x,1,16)==0,"");return 0;}
