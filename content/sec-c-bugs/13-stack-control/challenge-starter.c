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
 (void)out;(void)cap;(void)offset;(void)cookie;(void)win;return 0;
}
int copy_record(uint8_t *frame,size_t offset,const uint8_t *record,size_t record_len) {
 (void)frame;(void)offset;(void)record;(void)record_len;return 0;
}
