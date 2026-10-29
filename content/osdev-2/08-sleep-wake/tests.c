void kmain(void) { WaitQueue q,r; Task t[10]; wait_init(&q); wait_init(&r); for(int i=0;i<10;i++) t[i]=(Task){i,READY};
 int ok=1; for(int i=0;i<8;i++) if(sleep_on(&q,&t[i])) ok=0; CHECK("fill queue",ok && t[7].state==BLOCKED,"enqueue failed");
 CHECK("reject duplicate and full",sleep_on(&r,&t[0])==-1 && sleep_on(&q,&t[8])==-1 && t[8].state==READY,"invalid sleep");
 for(int i=0;i<5;i++) if(wake_one(&q)!=&t[i] || t[i].state!=READY) ok=0; CHECK("FIFO wake",ok,"wrong order");
 for(int i=0;i<5;i++) if(sleep_on(&q,&t[i])) ok=0; for(int i=5;i<8;i++) if(wake_one(&q)!=&t[i]) ok=0; for(int i=0;i<5;i++) if(wake_one(&q)!=&t[i]) ok=0;
 CHECK("wrap and empty",ok && wake_one(&q)==NULL,"wrap failed"); CHECK("independent queue",sleep_on(&r,&t[9])==0 && wake_one(&r)==&t[9],"other queue failed"); }
