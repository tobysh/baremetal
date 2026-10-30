#include <stdint.h>
#include <stddef.h>
#include <string.h>

size_t make_payload(uint8_t *out, size_t cap, size_t offset, uint64_t win) {
 if (!out || offset > SIZE_MAX-8 || cap < offset+8) return 0;
 memset(out, 'A', offset);
 for (size_t i=0;i<8;i++) out[offset+i]=(uint8_t)(win>>(8*i));
 return offset+8;
}
int safe_copy(uint8_t *frame, size_t offset, const uint8_t *input, size_t n) {
 if (n>offset) return 0;
 if (n==0) return 1;
 if (!frame || !input) return 0;
 memcpy(frame,input,n); return 1;
}
int main(void) {return 0;}

size_t make_guarded_payload(uint8_t *out,size_t cap,size_t offset,uint64_t cookie,uint64_t win) {
 if(!out||offset>SIZE_MAX-16||cap<offset+16)return 0;
 memset(out,'A',offset);
 for(size_t i=0;i<8;i++){out[offset+i]=(uint8_t)(cookie>>(8*i));out[offset+8+i]=(uint8_t)(win>>(8*i));}
 return offset+16;
}
int copy_record(uint8_t *frame,size_t offset,const uint8_t *record,size_t record_len) {
 if(!record||record_len<2)return 0;
 size_t n=(size_t)record[0]|((size_t)record[1]<<8);
 if(n!=record_len-2||n>offset||(!frame&&n))return 0;
 if(n)memcpy(frame,record+2,n);
 return 1;
}
