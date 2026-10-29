static const uint8_t u0[]={156,65,0,53,0,8,0,0};
static const uint8_t u1[]={0,53,195,80,0,11,0,0,97,98,99};
static const uint8_t u2[]={0,123,255,255,0,40,0,0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
static const uint8_t u3[]={156,65,0,53,0,10,0,0,97,98,116,97,105,108};
static const uint8_t u4[]={156,65,0,53,0,0,0,0};
static const uint8_t u5[]={156,65,0,53,0,7,0,0};
static const uint8_t u6[]={156,65,0,53,255,255,0,0};
static const uint8_t u7[]={0};
static const uint8_t u8[]={156,65,0,53,0,8,0};
static const struct {const uint8_t *p;size_t n;struct {int rc;unsigned src,dst;size_t payload;} want;} V[]={
{u0,8,{0,40001,53,0}},
{u1,11,{0,53,50000,3}},
{u2,40,{0,123,65535,32}},
{u3,14,{0,40001,53,2}},
{u4,8,{-1,0,0,0}},
{u5,8,{-1,0,0,0}},
{u6,8,{-1,0,0,0}},
{u7,0,{-1,0,0,0}},
{u8,7,{-1,0,0,0}}
};

int main(void){for(size_t i=0;i<sizeof V/sizeof V[0];i++){UDP u={0};int rc=parse_udp(V[i].p,V[i].n,&u);CHECK("UDP declared length",rc==V[i].want.rc && (rc || (u.src==V[i].want.src&&u.dst==V[i].want.dst&&u.payload==V[i].want.payload)),"datagram %zu",i);}return 0;}
