Encode field by field: `put_u16(out, 0xB4DE)`, then the nibble byte, then `out[3] = h->type`, `put_u16(out + 4, h->length)`, `put_u32(out + 6, h->seq)`.
===
In `hdr_decode`, check `n < 10` first, then `get_u16(buf) != 0xB4DE`. Only after both checks write to `*h`.
===
```c
if (n < HDR_LEN) return 0;
if (get_u16(buf) != HDR_MAGIC) return -1;
h->version = buf[2] >> 4;
h->flags   = buf[2] & 0x0F;
h->type    = buf[3];
h->length  = get_u16(buf + 4);
h->seq     = get_u32(buf + 6);
return HDR_LEN;
```
