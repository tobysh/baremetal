void kmain(void) {
 frame_init(); frame_reserve(0,2); frame_reserve(62,1000); frame_reserve(99,0xffffffffU);
 CHECK("reserved and first free", frame_take()==BASE+8192, "wrong first frame");
 CHECK("unaligned and outside rejected", frame_release(BASE+1)==-1 && frame_release(BASE-4096)==-1 && frame_release(BASE+64*4096ULL)==-1, "invalid free accepted");
 CHECK("free and reuse", frame_release(BASE+8192)==0 && frame_release(BASE+8192)==-1 && frame_take()==BASE+8192, "reuse failed");
 unsigned n=1; while(frame_take()) n++; CHECK("exhaustion", n==60 && frame_take()==0, "count %u", n);
 frame_init(); CHECK("reset", frame_take()==BASE, "not reset");
}
