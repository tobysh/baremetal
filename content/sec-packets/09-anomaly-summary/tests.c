static const Flow flows[]={{3405803785u,1,2},{3221225994u,443,2},{3325256711u,22,2},{3221225994u,80,2},{3325256711u,25,2},{3325256711u,9000,16},{3405803785u,1,2},{3221225994u,80,2},{3325256711u,80,2},{3405803785u,6,2},{3325256711u,21,2},{3325256711u,21,2},{3325256711u,21,2},{3405803785u,3,2},{3405803785u,5,2},{3405803785u,2,2},{3405803785u,1,2},{3221225994u,80,2},{3325256711u,21,2},{3405803785u,1,2},{3325256711u,8080,18},{3405803785u,4,2},{3221225994u,9000,16},{3405803785u,8080,18},{3325256711u,23,2},{3221225994u,8080,18},{3221225994u,80,2},{3405803785u,9000,16}};

int main(void){uint32_t out[8];size_t n=sizeof flows/sizeof flows[0];
 for(size_t threshold=1;threshold<=7;threshold++){size_t count=scan_sources(flows,n,threshold,out,8);uint32_t want[3];size_t k=0;if(threshold<=2)want[k++]=0xc000020a;if(threshold<=5)want[k++]=0xc6336407;if(threshold<=6)want[k++]=0xcb007109;
 CHECK("distinct SYN ports",count==k && !memcmp(out,want,k*sizeof *out),"threshold %zu returned %zu",threshold,count);}
 out[1]=0xdeadbeef;CHECK("output capacity",scan_sources(flows,n,3,out,1)==1 && out[0]==0xc6336407 && out[1]==0xdeadbeef,"");
 CHECK("empty or disabled",scan_sources(NULL,0,3,out,8)==0 && scan_sources(flows,n,0,out,8)==0 && scan_sources(flows,n,1,out,0)==0,"");return 0;}
