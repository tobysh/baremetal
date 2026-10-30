#include <stdint.h>
#include <stddef.h>
#include <string.h>

size_t make_payload(uint8_t *out, size_t cap, size_t offset, uint64_t win) {
 (void)out; (void)cap; (void)offset; (void)win; return 0;
}
int safe_copy(uint8_t *frame, size_t offset, const uint8_t *input, size_t n) {
 (void)frame; (void)offset; (void)input; (void)n; return 0;
}
int main(void) {return 0;}
