Keep Question as supplied. Implement `int dns_question(const uint8_t *p,size_t n,Question *out)` returning 0 or -1. Require n>=12 and QDCOUNT=1; decode an uncompressed first question with supported ASCII label characters, <=63 bytes per label and <=253 display characters. Root displays as `.`. Decode id, type, qclass. Require complete type/class, allow trailing data and leave out unchanged on failure.

Tests use several inputs, including boundary values. Keep the supplied function names and types.
