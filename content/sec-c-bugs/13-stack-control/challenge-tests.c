#include <stdlib.h>
static uint64_t decode_slot(const uint8_t *p){uint64_t n=0;for(size_t i=0;i<8;i++)n|=(uint64_t)p[i]<<(8*i);return n;}
int main(void){
 for(size_t offset=0;offset<=80;offset+=5){uint8_t payload[112],frame[128],before[128];memset(payload,0xcc,sizeof payload);
 uint64_t cookie=UINT64_C(0x007f00aa12003400)+offset,win=UINT64_C(0x0040100000000000)+offset*257;
 size_t n=make_guarded_payload(payload,sizeof payload,offset,cookie,win);int ok=n==offset+16;
 for(size_t j=offset+16;j<sizeof payload;j++)if(payload[j]!=0xcc)ok=0;
 CHECK("payload shape and trailing guards",ok,"offset=%zu",offset);
 memset(frame,0xa5,sizeof frame);if(n<=sizeof payload)memcpy(frame,payload,n);
 CHECK("canary preserved and return redirected",decode_slot(frame+offset)==cookie&&decode_slot(frame+offset+8)==win,"offset=%zu",offset);
 memset(payload,0xcc,sizeof payload);memcpy(before,payload,sizeof payload);
 CHECK("short capacity is atomic",make_guarded_payload(payload,offset+15,offset,cookie,win)==0&&memcmp(payload,before,sizeof payload)==0,"");
 }
 uint8_t b[32];memset(b,0x5a,sizeof b);
 CHECK("overflow and NULL payload output",make_guarded_payload(b,sizeof b,SIZE_MAX-8,0,0)==0&&b[0]==0x5a&&make_guarded_payload(NULL,32,8,0,0)==0,"");
 int record_cases_ok=1;
 for(size_t offset=0;offset<24;offset++)for(size_t n=0;n<32;n++){
 uint8_t frame[64],before[64],record[40];record[0]=(uint8_t)n;record[1]=0;for(size_t j=0;j<n;j++)record[2+j]=(uint8_t)(j*17);
 memset(frame,0xa5,sizeof frame);memcpy(before,frame,sizeof frame);int rc=copy_record(frame,offset,record,n+2);int ok=rc==(n<=offset);
 if(n<=offset){if(memcmp(frame,record+2,n))ok=0;for(size_t j=n;j<sizeof frame;j++)if(frame[j]!=0xa5)ok=0;}else if(memcmp(frame,before,sizeof frame))ok=0;
 if(!ok)record_cases_ok=0;
 }
 CHECK("768 record layouts respect the local buffer",record_cases_ok,"a record result or guard byte differed");
 uint8_t short_record[]={3,0,'a','b'},trailing[]={1,0,'a','x'},zero[]={0,0},one[]={1,0,'x'};
 uint8_t saved[32];memcpy(saved,b,sizeof b);
 CHECK("truncation leaves memory unchanged",copy_record(b,32,short_record,sizeof short_record)==0&&memcmp(b,saved,sizeof b)==0,"");
 CHECK("trailing data leaves memory unchanged",copy_record(b,32,trailing,sizeof trailing)==0&&memcmp(b,saved,sizeof b)==0,"");
 CHECK("invalid headers and pointers",copy_record(b,32,NULL,0)==0&&copy_record(b,32,zero,0)==0&&copy_record(b,32,zero,1)==0&&copy_record(NULL,32,one,3)==0,"");
 CHECK("empty payload needs no destination",copy_record(NULL,0,zero,2)==1,"");
 uint8_t *large=malloc(65537),*dest=malloc(65551);int ok=large&&dest;
 if(ok){memset(large,0x7c,65537);large[0]=255;large[1]=255;memset(dest,0xa5,65551);ok=copy_record(dest,65535,large,65537)==1&&memcmp(dest,large+2,65535)==0;for(size_t j=65535;j<65551;j++)if(dest[j]!=0xa5)ok=0;}
 CHECK("maximum 16-bit length and guard bytes",ok,"");free(large);free(dest);return 0;
}
