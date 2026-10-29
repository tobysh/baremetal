void kmain(void) { heap_init(); void *a=heap_alloc(9), *b=heap_alloc(24), *c=heap_alloc(16);
 CHECK("alignment and splitting", a && b && c && (uintptr_t)a%8==0 && (uintptr_t)b-(uintptr_t)a==16, "bad allocations");
 CHECK("bad frees", heap_free((char*)b+1)==-1 && heap_free(NULL)==-1, "invalid pointer accepted");
 CHECK("free and double free", heap_free(b)==0 && heap_free(b)==-1, "bad free result");
 CHECK("first fit", heap_alloc(20)==b, "did not reuse hole");
 heap_init(); a=heap_alloc(80); b=heap_alloc(80); c=heap_alloc(80); heap_free(b); heap_free(a); CHECK("coalescing", heap_alloc(160)==a, "neighbours not merged");
 heap_init(); CHECK("invalid size", heap_alloc(0)==NULL && heap_alloc((size_t)-1)==NULL, "invalid allocation"); CHECK("whole arena", heap_alloc(256)!=NULL && heap_alloc(1)==NULL, "exhaustion failed");
}
