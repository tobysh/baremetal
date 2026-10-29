static const uint8_t e0[]={2,0,0,0,0,2,2,0,0,0,0,1,8,0};
static const uint8_t e1[]={2,0,0,0,0,2,2,0,0,0,0,1,129,0,0,42,8,0,226};
static const uint8_t e2[]={2,0,0,0,0,2,2,0,0,0,0,1,8,6,115,178};
static const uint8_t e3[]={2,0,0,0,0,2,2,0,0,0,0,1,129,0,0,42,8,6,235,145,92};
static const uint8_t e4[]={2,0,0,0,0,2,2,0,0,0,0,1,134,221,234,217,44,138};
static const uint8_t e5[]={2,0,0,0,0,2,2,0,0,0,0,1,129,0,0,42,134,221,133,75,216,118,68};
static const uint8_t e6[]={2,0,0,0,0,2,2,0,0,0,0,1,136,181,130,185,53,125,43,160};
static const uint8_t e7[]={2,0,0,0,0,2,2,0,0,0,0,1,129,0,0,42,136,181,143,100,225,202,103,101,62};
static const uint8_t e8[]={0};
static const uint8_t e9[]={2,0,0,0,0,2,2,0,0,0,0,1,8};
static const uint8_t e10[]={2,0,0,0,0,2,2,0,0,0,0,1,129,0,0,1,8};
static const struct {const uint8_t *p;size_t n;struct {int rc;unsigned type;size_t off;} want;} V[]={
{e0,14,{0,2048,14}},
{e1,19,{0,2048,18}},
{e2,16,{0,2054,14}},
{e3,21,{0,2054,18}},
{e4,18,{0,34525,14}},
{e5,23,{0,34525,18}},
{e6,20,{0,34997,14}},
{e7,25,{0,34997,18}},
{e8,0,{-1,0,0}},
{e9,13,{-1,0,0}},
{e10,17,{-1,0,0}}
};

int main(void){for(size_t i=0;i<sizeof V/sizeof V[0];i++){Ethernet e={0};int rc=parse_ethernet(V[i].p,V[i].n,&e);int ok=rc==V[i].want.rc;
 if(rc==0)ok=ok && e.type==V[i].want.type && e.payload==V[i].want.off && !memcmp(e.dst,V[i].p,6) && !memcmp(e.src,V[i].p+6,6);
 CHECK("Ethernet fields",ok,"frame %zu",i);}return 0;}
