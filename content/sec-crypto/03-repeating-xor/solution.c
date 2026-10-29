#include <stdio.h>
#include <string.h>

int main(void) {
    const char *message = "Burn the letters";
    const char *key = "ICE";

    size_t klen = strlen(key);
    for (size_t i = 0; message[i] != '\0'; i++) {
        unsigned char c = (unsigned char)(message[i] ^ key[i % klen]);
        printf("%02x", c);
    }
    printf("\n");
    return 0;
}
