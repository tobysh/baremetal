#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BAD_SECURE   1u   // some Set-Cookie lacks Secure
#define BAD_HTTPONLY 2u   // some Set-Cookie lacks HttpOnly
#define BAD_SAMESITE 4u   // some Set-Cookie lacks a valid SameSite
#define BAD_HSTS     8u   // no Strict-Transport-Security, or max-age under 180 days
#define BAD_NOSNIFF  16u  // no X-Content-Type-Options: nosniff

// Look at one canned HTTP response and return the OR of the BAD_* flags that apply.
// TODO: this stub reports nothing, so every response looks safe.
unsigned audit_response(const char *response) {
    (void)response;
    return 0;
}

int main(void) {
    const char *r = "HTTP/1.1 200 OK\r\nSet-Cookie: sid=abc; Path=/\r\n\r\nhello";
    printf("%u\n", audit_response(r));
    return 0;
}
