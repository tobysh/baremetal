int main(void){for(size_t n=1;n<=128;n*=2){void *p=malloc(n);release(&p);int cleared=p==NULL;
 CHECK("owner cleared",cleared,"allocation size %zu",n);
 if(cleared){release(&p);CHECK("second cleanup harmless",p==NULL,"");}}
 void *p=NULL;release(&p);release(NULL);CHECK("empty owners",p==NULL,"");return 0;}
