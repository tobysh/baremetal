static const uint8_t t0[]={156,64,0,80,0,0,3,232,0,0,0,0,80,2,16,0,0,0,0,0};
static const uint8_t t1[]={0,80,156,64,0,0,3,233,0,0,3,231,80,24,16,0,0,0,0,0,104,101,108,108,111};
static const uint8_t t2[]={255,255,1,187,255,255,255,255,128,0,0,0,128,18,16,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,97,98};
static const uint8_t t3[]={0,1,0,2,0,0,0,0,0,0,0,0,240,1,16,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
static const uint8_t t4[]={156,64,0,80,0,0,3,232,0,0,0,0,64,2,16,0,0,0,0,0};
static const uint8_t t5[]={156,64,0,80,0,0,3,232,0,0,0,0,240,2,16,0,0,0,0,0};
static const uint8_t t6[]={0};
static const uint8_t t7[]={156,64,0,80,0,0,3,232,0,0,0,0,80,2,16,0,0,0,0};
static const struct {const uint8_t *p;size_t n;struct {int rc;unsigned src,dst;uint32_t seq,ack;unsigned flags;size_t header,payload;} want;} V[]={
{t0,20,{0,40000,80,1000u,0u,2,20,0}},
{t1,25,{0,80,40000,1001u,999u,24,20,5}},
{t2,34,{0,65535,443,4294967295u,2147483648u,18,32,2}},
{t3,60,{0,1,2,0u,0u,1,60,0}},
{t4,20,{-1,0,0,0,0,0,0,0}},
{t5,20,{-1,0,0,0,0,0,0,0}},
{t6,0,{-1,0,0,0,0,0,0,0}},
{t7,19,{-1,0,0,0,0,0,0,0}}
};

int main(void){for(size_t i=0;i<sizeof V/sizeof V[0];i++){TCP t={0};int rc=parse_tcp(V[i].p,V[i].n,&t);int ok=rc==V[i].want.rc;if(!rc)ok=ok&&t.src==V[i].want.src&&t.dst==V[i].want.dst&&t.seq==V[i].want.seq&&t.ack==V[i].want.ack&&t.flags==V[i].want.flags&&t.header==V[i].want.header&&t.payload==V[i].want.payload;CHECK("TCP header fields",ok,"segment %zu",i);}return 0;}
