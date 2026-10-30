The stored checksum covers decoded bytes, so recover bytes before checking it.
===
Validate header size and declared length before slicing. Encoding and checksums are separate from authentication.
