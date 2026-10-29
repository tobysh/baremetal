#!/usr/bin/env python3
"""Reproduce the authored security lessons; fixture generators remain separate.

Only writes sec-c-bugs, sec-crypto and sec-forensics lesson files. Existing
Claude-authored lessons and generated forensic harnesses are retained.
"""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2] / 'content'
HEAD = '#include <stdio.h>\n#include <stdint.h>\n#include <stddef.h>\n#include <stdlib.h>\n#include <string.h>\n#include <limits.h>\n#include <time.h>\n'


def lesson(track, folder, title, summary, body, task, solution, starter, tests=None,
           hints=(), difficulty='medium', mode='harness', review=False):
    d = ROOT / track / folder
    d.mkdir(parents=True, exist_ok=True)
    slug = folder.split('-', 1)[1]
    meta = f'---\ntitle: {title}\nsummary: {summary}\nminutes: 25\ndifficulty: {difficulty}\ntags: security, defensive-programming\nid: {track}/{slug}\nmode: {mode}\n'
    if review:
        meta += 'kind: review\n'
    vulnerable = {
        '03-size-overflow': ('size_t bytes = count * width + 1;\nvoid *p = malloc(bytes);',
                             'The multiplication can wrap before malloc sees the request. A small allocation does not justify a later count-element write.'),
        '04-signed-unsigned': ('int len = parse_length(); /* may return -1 */\nmemcpy(dst, src, (size_t)len);',
                               'The cast turns an error into a huge request. Reject negative values while they are still signed.'),
        '05-format-strings': ('snprintf(dst, cap, user_text);',
                              'Destination bounds do not make an attacker-controlled format safe. Keep the format literal and supply user text as data.'),
        '06-use-after-free': ('char *p = malloc(n + 1);\n/* populate p */\nfree(p);\nreturn p; /* dangling */',
                             'A later allocation can reuse the same bytes. A caller reading this pointer may disclose unrelated data or act on corrupted state. The returned owner must stay live.'),
        '07-double-free': ('void cleanup(void *p) {\n    free(p);\n    p = NULL; /* only changes a local copy */\n}',
                          'The caller still holds the freed address. Repeating cleanup may abort or corrupt allocator state; clear the actual owning slot.'),
    }
    if track == 'sec-c-bugs' and folder in vulnerable:
        snippet, explanation = vulnerable[folder]
        body += '\n\n## The unsafe pattern\n\n```c\n' + snippet + '\n```\n\n' + explanation
    (d / 'lesson.md').write_text(meta + '---\n' + body.strip() + '\n')
    (d / 'task.md').write_text(task.strip() + '\n\nTests use several inputs, including boundary values. Keep the supplied function names and types.\n')
    (d / 'hints.md').write_text('\n===\n'.join(hints) + '\n')
    (d / 'solution.c').write_text(HEAD + solution.strip() + '\n')
    (d / 'starter.c').write_text(HEAD + starter.strip() + '\n')
    if tests is not None:
        (d / 'tests.c').write_text(tests.strip() + '\n')


def bugs():
    track = 'sec-c-bugs'
    lesson(track, '02-unbounded-copy', 'Copying untrusted bytes',
           'Repair an unbounded copy by checking the destination capacity before changing memory.',
           '''An unbounded `memcpy(dst, src, n)` trusts the caller's length. A packet length or file field is a claim, not proof that the destination has that much room. An oversized copy can overwrite a neighbouring object even when the source contains no zero bytes.

Here the interface has two independent bounds: the source contains `n` readable bytes, and the destination has `cap` writable bytes. A byte copy differs from a string copy: there is no terminator and embedded zeros are ordinary data. Decide whether the entire request fits before copying anything. This exercise rejects an oversized request rather than silently truncating it.

Treat zero length as a successful no-op, including when either pointer is NULL. For a nonempty request, both pointers must be non-NULL. The buffers do not overlap; overlapping moves would need `memmove`. A failed operation must leave every destination byte unchanged, so validation belongs before the write. In review, look for every route from an external length to a memory operation and identify the actual destination capacity.''',
           'Implement `int copy_bytes(uint8_t *dst, size_t cap, const uint8_t *src, size_t n)`. Return 1 on success, 0 on rejection. Reject `n > cap` or a NULL pointer for nonzero `n`. Copy exactly `n` bytes, without a terminator. Failure writes nothing; `n == 0` succeeds.',
           '''int copy_bytes(uint8_t *dst,size_t cap,const uint8_t *src,size_t n) {
    if(n>cap || (n && (!dst || !src))) return 0;
    if(n) memcpy(dst,src,n);
    return 1;
}
int main(void){return 0;}''',
           '''int copy_bytes(uint8_t *dst,size_t cap,const uint8_t *src,size_t n) {
    (void)cap;
    if(!dst || !src) return 0;
    /* TODO: check the capacity before this unbounded copy. */
    memcpy(dst,src,n); return 1;
}
int main(void){return 0;}''',
           '''int main(void){
 uint8_t s[12]={1,0,3,4,5,6,7,8,9,10,11,12};
 for(size_t cap=0;cap<=8;cap++) for(size_t n=0;n<=12;n++){
  uint8_t d[32]; memset(d,0xa5,sizeof d);
  int rc=copy_bytes(d+4,cap,s,n); int ok=rc==(n<=cap);
  for(size_t i=0;i<32;i++) {uint8_t want=(n<=cap && i>=4 && i<4+n)?s[i-4]:0xa5; if(d[i]!=want)ok=0;}
  CHECK("length and guard bytes",ok,"cap=%zu n=%zu",cap,n);
 }
 CHECK("zero bytes allow NULL",copy_bytes(NULL,0,NULL,0)==1,"");
 uint8_t d=42;
 CHECK("NULL source rejected",copy_bytes(&d,1,NULL,1)==0 && d==42,"");
 CHECK("NULL destination rejected",copy_bytes(NULL,1,s,1)==0,""); return 0;
}''', hints=('Compare `n` with `cap` before calling `memcpy`.', 'Only require non-NULL pointers when `n != 0`. Return before any copy on failure.'), difficulty='easy')

    lesson(track, '03-size-overflow', 'Allocation size overflow',
           'Check a multiplication and an added terminator before an allocation size can wrap.',
           '''A common allocation is `malloc(count * width + 1)`. Unsigned arithmetic wraps modulo one more than `SIZE_MAX`: the expression can become tiny while later code still believes it allocated `count` elements. Checking the result after multiplying is too late.

Work backward from the maximum permitted result. Reserving one trailing byte leaves `SIZE_MAX - 1` bytes for elements. When `width` is nonzero, the product fits exactly when `count <= (SIZE_MAX - 1) / width`. Division happens before multiplication and cannot overflow. A zero width is valid here and yields a one-byte allocation size.

This exercise computes a size without allocating, so even enormous values are safe to test. The function also leaves the caller's output untouched on failure. In a real allocation path, check `malloc` separately: representable does not mean affordable. Apply quotas as well as arithmetic checks. Signed overflow follows different rules and is undefined behaviour, which is another reason to use `size_t` for sizes and validate conversions at the boundary.''',
           'Implement `int allocation_size(size_t count, size_t width, size_t *out)`: compute `count * width + 1`, returning 1 when representable and 0 otherwise. NULL `out` is failure. On failure do not change `*out`; do not allocate memory.',
           '''int allocation_size(size_t count,size_t width,size_t *out){
 if(!out || (width && count>(SIZE_MAX-1)/width)) return 0;
 *out=count*width+1; return 1;
}
int main(void){return 0;}''',
           '''int allocation_size(size_t count,size_t width,size_t *out){
 if(!out)return 0; *out=count*width+1; return 1;
}
int main(void){return 0;}''',
           '''int main(void){
 const size_t a[][2]={{0,0},{0,SIZE_MAX},{3,4},{SIZE_MAX,0},{SIZE_MAX,1},{SIZE_MAX/2,2},{SIZE_MAX/2+1,2},{1,SIZE_MAX},{1,SIZE_MAX-1},{SIZE_MAX,8}};
 for(size_t i=0;i<sizeof a/sizeof a[0];i++){size_t c=a[i][0],w=a[i][1],out=77;int want=!w || c<=(SIZE_MAX-1)/w;int rc=allocation_size(c,w,&out);
 CHECK("size arithmetic",rc==want && out==(want?c*w+1:77),"case %zu",i);}
 CHECK("NULL output",allocation_size(1,1,NULL)==0,"");return 0;
}''', hints=('Reserve the extra byte first: the multiplication must be at most `SIZE_MAX - 1`.', 'Handle `width == 0` before division, then reject when `count > (SIZE_MAX - 1) / width`.'))

    lesson(track, '04-signed-unsigned', 'Signed lengths at a boundary',
           'Reject negative lengths before converting them to size_t or copying bytes.',
           '''A parser may return `-1` for an error. Passing that value to `memcpy`, whose length is `size_t`, converts it into a huge unsigned number. A comparison such as `len <= cap` can also perform a conversion before you expected it. Follow the value through its type changes.

Keep the signed input signed until you have established `len >= 0`. Only then convert it to `size_t` and compare it with the destination capacity. Our interface returns the copied length, or -1 for failure. Success with length zero is valid even for NULL pointers.

The source is guaranteed to have `len` readable bytes when `len` is positive; this function can validate the destination capacity but cannot infer the source allocation size from a pointer. Production interfaces should carry both bounds when that guarantee is unavailable. This separation between a caller guarantee and a checked condition helps make a review precise. Do not fix a negative length by taking its absolute value: `INT_MIN` cannot be negated in an `int`, and an error is not a valid request.''',
           'Implement `int copy_signed(uint8_t *dst, size_t cap, const uint8_t *src, int len)`. Return -1 and write nothing for negative length, insufficient capacity, or NULL pointer with positive length. Otherwise copy exactly `len` bytes and return `len`; zero succeeds.',
           '''int copy_signed(uint8_t *dst,size_t cap,const uint8_t *src,int len){
 if(len<0)return -1;
 size_t n=(size_t)len;
 if(n>cap || (n && (!dst || !src)))return -1;
 if(n)memcpy(dst,src,n);return len;
}
int main(void){return 0;}''',
           '''int copy_signed(uint8_t *dst,size_t cap,const uint8_t *src,int len){
 /* TODO: distinguish errors from a valid zero-byte request. */
 (void)dst;(void)cap;(void)src;return len;
}
int main(void){return 0;}''',
           '''int main(void){uint8_t s[10]={0,1,2,3,4,5,6,7,8,9};int lens[]={INT_MIN,-20,-1,0,1,4,8,10};
 for(size_t c=0;c<=8;c++)for(size_t j=0;j<8;j++){uint8_t d[32];memset(d,0x55,sizeof d);int n=lens[j],good=n>=0 && (size_t)n<=c;int rc=copy_signed(d+4,c,s,n),ok=rc==(good?n:-1);
 for(size_t k=0;k<32;k++)if(d[k]!=(good&&k>=4&&k<4+(size_t)n?s[k-4]:0x55))ok=0;
 CHECK("signed boundary",ok,"cap=%zu length=%d",c,n);}
 CHECK("zero allows NULL",copy_signed(NULL,0,NULL,0)==0,"");
 CHECK("positive requires pointers",copy_signed(NULL,4,s,1)==-1 && copy_signed(s,4,NULL,1)==-1,"");return 0;}''', hints=('Test `len < 0` before any cast to `size_t`.', 'After the sign check, compare the converted length to `cap`, then validate pointers and copy.'), difficulty='easy')

    lesson(track, '05-format-strings', 'Data must stay data',
           'Fix a format-string boundary so percent directives in user text are printed literally.',
           '''The first argument of `printf` is a miniature program. `%s` fetches a pointer, `%x` fetches an integer, and `%n` writes through a pointer. Calling `printf(user_text)` lets untrusted text choose operations and consume arguments that were never supplied. It is a format-string vulnerability even if the buffer containing the text is perfectly bounded.

Keep the format a literal you control and pass the text as a value: `printf("%s", user_text)`. The same rule applies to `snprintf`; a bounded destination does not make an uncontrolled format safe. Here we build a bounded log message using a literal `"%s"` format. Percent signs, newlines and empty strings stay ordinary bytes.

`snprintf` returns the number of characters that would have been written, excluding the terminator, even if the buffer was too small. That result is useful for detecting truncation. Our wrapper returns this required length as `size_t`, and treats a NULL text as empty. This is only a formatting repair: log injection through newlines is a separate policy question. The exercise deliberately preserves text exactly rather than introducing escaping rules.''',
           'Implement `size_t literal_message(char *dst, size_t cap, const char *text)`. Store text literally, truncating to `cap-1` bytes and terminating when `cap > 0`. Return the full text length. NULL text means empty; for `cap == 0`, write nothing and allow NULL dst. For positive cap the caller supplies a valid destination.',
           '''size_t literal_message(char *dst,size_t cap,const char *text){
 if(!text)text="";
 int n=snprintf(dst,cap,"%s",text);return n<0?0:(size_t)n;
}
int main(void){return 0;}''',
           '''size_t literal_message(char *dst,size_t cap,const char *text){
 /* TODO: preserve all of text rather than dropping it. Never use text as a format. */
 (void)text;if(cap)dst[0]=0;return 0;
}
int main(void){return 0;}''',
           r'''int main(void){const char *s[]={"plain","%s %x %n","100% complete","%%","","line\nnext",NULL};
 for(size_t j=0;j<7;j++)for(size_t cap=0;cap<=14;cap++){char d[32];memset(d,'Z',sizeof d);const char *p=s[j]?s[j]:"";size_t n=strlen(p),k=cap?(n<cap-1?n:cap-1):0;size_t rc=literal_message(d+2,cap,s[j]);
 int ok=rc==n && (!cap || (memcmp(d+2,p,k)==0 && d[2+k]==0));for(size_t x=0;x<32;x++)if((x<2 || x>=2+cap)&&d[x]!='Z')ok=0;
 CHECK("literal formatting and bounds",ok,"text %zu cap %zu",j,cap);}
 CHECK("sizing call",literal_message(NULL,0,"%n")==2,"");return 0;}''', hints=('The format argument should be the literal `"%s"`.', '`snprintf(dst, cap, "%s", text)` already provides truncation, termination and the required length.'), difficulty='easy')

    lesson(track, '06-use-after-free', 'Keep an owned copy alive',
           'Repair a lifetime error by copying data while the owner is alive and returning the new owner.',
           '''A pointer is an address, not ownership. Returning a pointer after freeing its allocation leaves the caller holding a dangling pointer. Reading it is undefined behaviour even if a quick run still prints the old text. The allocator is free to reuse the bytes immediately.

Our function must return a fresh owned copy of a label. Compute its size while the input is alive, allocate a separate buffer, copy the terminator as well as the characters, and return that allocation without freeing it. The caller now owns the result and will call `free` once. A NULL input has no label and returns NULL.

Do not return the input pointer: it might refer to a stack array or read-only string literal, and the caller is promised ownership. Also do not return a local array, whose lifetime ends at return. Writing an ownership sentence beside each public pointer parameter and return value often catches these mistakes before tests do. Allocation failure must propagate as NULL; callers cannot safely use a buffer that was never allocated. These rules apply equally to early-return and error paths.''',
           'Implement `char *owned_label(const char *src)`. Return an independent heap allocation containing the complete string and terminator, or NULL for NULL input/allocation failure. Caller frees the result. Do not modify src or free the returned allocation.',
           '''char *owned_label(const char *src){
 if(!src)return NULL;size_t n=strlen(src);if(n==SIZE_MAX)return NULL;
 char *p=malloc(n+1);if(!p)return NULL;memcpy(p,src,n+1);return p;
}
int main(void){return 0;}''',
           '''char *owned_label(const char *src){
 /* TODO: allocate a copy with a lifetime owned by the caller. */
 (void)src;return NULL;
}
int main(void){return 0;}''',
           '''int main(void){const char *s[]={"","a","longer label","percent %n and spaces"};
 for(size_t i=0;i<4;i++){char src[100];strcpy(src,s[i]);char *a=owned_label(src),*b=owned_label(src);int ok=a && b && a!=b && a!=src && b!=src && !strcmp(a,s[i]) && !strcmp(b,s[i]);
 src[0]='X';if(a && strcmp(a,s[i]))ok=0;if(a && a[0]){a[0]='!';if(b && strcmp(b,s[i]))ok=0;}
 CHECK("independent live owners",ok,"input %zu",i);free(a);free(b);}
 CHECK("NULL has no owner",owned_label(NULL)==NULL,"");return 0;}''', hints=('Use `strlen(src) + 1` bytes for the terminator.', 'Allocate, check for NULL, copy, and return. The caller performs the free.'))

    lesson(track, '07-double-free', 'Make release repeatable',
           'Clear an owning pointer after release so repeated cleanup cannot free the same allocation twice.',
           '''Cleanup code is often reached through several paths. If two branches both call `free(p)`, the second call uses an address whose allocation is already gone. That can corrupt allocator state or terminate the process. A defensive cleanup helper can make repeated release through the same owner harmless.

C passes pointers by value. Setting a local parameter to NULL does not change the caller's pointer. Accept `void **owner` so the helper can replace the pointer stored by the caller. Free its value once, then store NULL. `free(NULL)` is defined to do nothing, so the second call becomes a safe no-op. Also accept a NULL owner address.

This pattern only protects the one owning slot. Copies of the address elsewhere remain dangling; it does not implement reference counting or make shared ownership safe. Prefer one clear owner and borrowed pointers whose lifetime stays within that owner. In larger structs, release each owned field then reset its length/capacity metadata. A destructor should leave an object in a documented state that can be inspected and cleaned again.''',
           'Implement `void release(void **owner)`. For a non-NULL owner address, free `*owner` and replace it with NULL. A NULL owner address or already-NULL value is harmless. Repeated calls through the same owner must be safe.',
           '''void release(void **owner){if(!owner)return;free(*owner);*owner=NULL;}
int main(void){return 0;}''',
           '''void release(void **owner){
 if(!owner)return;free(*owner); /* TODO: reset the caller's owner. */
}
int main(void){return 0;}''',
           '''int main(void){for(size_t n=1;n<=128;n*=2){void *p=malloc(n);release(&p);int cleared=p==NULL;
 CHECK("owner cleared",cleared,"allocation size %zu",n);
 if(cleared){release(&p);CHECK("second cleanup harmless",p==NULL,"");}}
 void *p=NULL;release(&p);release(NULL);CHECK("empty owners",p==NULL,"");return 0;}''', hints=('The caller supplies the address of its pointer, so change `*owner`.', 'Check owner, call `free(*owner)`, and assign `*owner = NULL`.'), difficulty='easy')

    lesson(track, '08-review-safe-buffers', 'Review: a defensive record copy',
           'Combine signed-length validation, safe size arithmetic, bounded copying and literal data handling.',
           '''This review combines the checks from [signed lengths](/learn/sec-c-bugs/signed-unsigned), [allocation overflow](/learn/sec-c-bugs/size-overflow) and [bounded copying](/learn/sec-c-bugs/unbounded-copy). A record arrives as a byte span plus a signed count. The caller wants its bytes followed by a zero byte.

Validate the sign before converting to `size_t`. Next reserve a terminator without computing `n + 1` too early: requiring `n < cap` proves both that the addition is representable and that the destination is large enough. Positive data lengths require a readable source, and every successful call requires a destination because even an empty result stores a terminator.

The record is bytes, not a C string, so an embedded zero is part of the payload and must not stop the copy. Percent characters also stay literal data. Validate everything before writing, then copy exactly the accepted count and append the terminator. Failures leave the destination unchanged. This is the same sequence used by more complicated parsers: establish the contract, prove the bounds, perform the write.''',
           'Implement `int record_copy(uint8_t *dst,size_t cap,const uint8_t *src,int len)`. Return 1 and copy exactly len bytes plus one zero terminator. Return 0 for negative len, NULL dst, NULL src with positive len, or insufficient capacity. A failure writes nothing. Source and destination do not overlap.',
           '''int record_copy(uint8_t *dst,size_t cap,const uint8_t *src,int len){
 if(len<0 || !dst)return 0;size_t n=(size_t)len;
 if(n>=cap || (n && !src))return 0;
 if(n)memcpy(dst,src,n);dst[n]=0;return 1;
}
int main(void){return 0;}''',
           '''int record_copy(uint8_t *dst,size_t cap,const uint8_t *src,int len){
 (void)dst;(void)cap;(void)src;(void)len;return 0;
}
int main(void){return 0;}''',
           '''int main(void){uint8_t src[12]={'%',0,'n',255,4,5,6,7,8,9,10,11};
 for(int len=-2;len<=12;len++)for(size_t cap=0;cap<=12;cap++){uint8_t out[32];memset(out,0xa5,sizeof out);int good=len>=0 && (size_t)len<cap;int rc=record_copy(out+3,cap,src,len),ok=rc==good;
 for(size_t i=0;i<32;i++){uint8_t w=0xa5;if(good&&i>=3&&i<3+(size_t)len)w=src[i-3];if(good&&i==3+(size_t)len)w=0;if(out[i]!=w)ok=0;}
 CHECK("record boundaries",ok,"len %d cap %zu",len,cap);}
 uint8_t d=9;CHECK("NULL source",record_copy(&d,1,NULL,0)==1 && d==0 && record_copy(&d,2,NULL,1)==0,"");
 CHECK("NULL dst",record_copy(NULL,2,src,1)==0,"");return 0;}''', hints=('This is a review: make every validation happen before the first write.', 'After rejecting a negative length, `n < cap` reserves the terminator without risky addition.'), review=True)


def crypto():
    track='sec-crypto'
    p=ROOT/track/'track.json'
    meta=json.loads(p.read_text())
    meta['description']='Encode bytes, experiment with repeating XOR and Caesar ciphers, predict a toy LCG, identify repeated blocks, explore a tiny hash collision and combine byte comparison with hex decoding. These deliberately weak constructions explain failure modes; they are not cryptographic tools. Use vetted cryptographic libraries and operating-system randomness in real applications.'
    p.write_text(json.dumps(meta,indent=2)+'\n')
    lesson(track,'07-detect-ecb','Spot repeated blocks',
           'Identify equal full blocks as evidence of ECB-like pattern leakage without implementing an encryption algorithm.',
           '''A block cipher transforms a fixed-size block under a key. In Electronic Codebook mode, equal plaintext blocks under the same key produce equal ciphertext blocks. This exposes structure even when individual bytes look random. Detection needs only byte comparison, not the key.

Split the input into full blocks of `block_size` bytes and compare every pair. A trailing partial block is ignored because it is not a complete cipher block. Return whether any pair matches. Do not compare only adjacent blocks: repetitions may be far apart. A quadratic scan is fine for these small fixtures and needs no allocations.

Repeated ciphertext blocks are evidence, not proof of a particular mode. Duplicated records, an unusual protocol or chance can also cause repetition. Conversely, a short ECB message may contain no repeats. The exercise accepts arbitrary block sizes to teach the reasoning; AES uses 16-byte blocks. No actual encryption occurs here. Real systems normally use a vetted authenticated-encryption construction with its required nonce policy, rather than selecting a mode from a toy detector.''',
           'Implement `int repeated_block(const uint8_t *p,size_t n,size_t block_size)`: return 1 if any two complete blocks are identical, else 0. Ignore a partial tail. Return 0 for block_size zero, NULL input, or fewer than two full blocks.',
           '''int repeated_block(const uint8_t *p,size_t n,size_t b){
 if(!p || !b)return 0;size_t blocks=n/b;
 for(size_t i=0;i<blocks;i++)for(size_t j=i+1;j<blocks;j++)if(!memcmp(p+i*b,p+j*b,b))return 1;
 return 0;
}
int main(void){return 0;}''',
           '''int repeated_block(const uint8_t *p,size_t n,size_t b){(void)p;(void)n;(void)b;return 0;}
int main(void){return 0;}''',
           '''int main(void){uint8_t x[96];for(size_t b=1;b<=24;b++){for(size_t i=0;i<96;i++)x[i]=(uint8_t)i;
 CHECK("distinct blocks",repeated_block(x,b*4,b)==0,"block size %zu",b);
 memcpy(x+3*b,x,b);CHECK("nonadjacent repeat",repeated_block(x,b*4,b)==1,"block size %zu",b);
 CHECK("partial tail ignored",repeated_block(x,b*4-1,b)==0,"block size %zu",b);}
 CHECK("invalid or short",repeated_block(NULL,100,16)==0 && repeated_block(x,96,0)==0 && repeated_block(x,1,16)==0,"");return 0;}''', hints=('Count complete blocks with `n / block_size` after rejecting zero.', 'Compare all pairs `i < j` using `memcmp(p + i*b, p + j*b, b)`.'),difficulty='easy')

    lesson(track,'08-toy-hash','Collisions in a tiny hash',
           'Find two distinct two-byte messages with the same deliberately tiny FNV-derived hash.',
           '''A hash maps arbitrary messages into a fixed output space. With only 256 outputs, 257 distinct inputs guarantee a collision by the pigeonhole principle. This lesson deliberately keeps only the low eight bits of FNV-1a so you can see the entire state space.

Start at `0x811c9dc5`, XOR each byte, multiply by `0x01000193` modulo 2^32, and finally keep `h & 255`. Use `uint32_t` so wraparound is defined. FNV is a noncryptographic hash even before this truncation: it is not a password hash, MAC, signature or integrity proof against an attacker.

To find a collision, enumerate every two-byte message in increasing little-endian integer order: value v represents `{v & 255, v >> 8}`. Remember the first v for each hash bucket. On the first occupied bucket, return the earlier and current values. The deterministic order makes the exercise reproducible; it is not a claim about breaking a modern cryptographic hash. Even a strong digest requires an authenticated reference to establish who created it, and plain hashes alone do not authenticate messages.''',
           'Implement `uint8_t tiny_hash(const uint8_t *p,size_t n)` using the FNV-1a constants above and low-eight-bit output. Implement `int first_collision(uint16_t *a,uint16_t *b)`: enumerate v=0..65535 as two little-endian bytes and return the first repeated bucket with its earliest value in *a and current value in *b. Return 1 on success, 0 for a NULL output.',
           '''uint8_t tiny_hash(const uint8_t *p,size_t n){uint32_t h=0x811c9dc5u;for(size_t i=0;i<n;i++)h=(h^p[i])*0x01000193u;return (uint8_t)h;}
int first_collision(uint16_t *a,uint16_t *b){if(!a||!b)return 0;int seen[256];for(int i=0;i<256;i++)seen[i]=-1;
 for(unsigned v=0;v<65536;v++){uint8_t p[2]={(uint8_t)v,(uint8_t)(v>>8)};unsigned h=tiny_hash(p,2);if(seen[h]>=0){*a=(uint16_t)seen[h];*b=(uint16_t)v;return 1;}seen[h]=(int)v;}return 0;}
int main(void){return 0;}''',
           '''uint8_t tiny_hash(const uint8_t *p,size_t n){(void)p;(void)n;return 0;}
int first_collision(uint16_t *a,uint16_t *b){(void)a;(void)b;return 0;}
int main(void){return 0;}''',
           '''int main(void){const uint8_t m[][4]={{0},{'a'},{'a','b'},{255,0,128,1}};size_t ns[]={0,1,2,4};unsigned wants[]={197,44,202,237};
 for(size_t i=0;i<4;i++)CHECK("tiny hash vector",tiny_hash(m[i],ns[i])==wants[i],"vector %zu",i);
 uint16_t a=0,b=0;int rc=first_collision(&a,&b);CHECK("first collision",rc==1 && a==239 && b==256,"got %u and %u",a,b);
 uint8_t x[2]={(uint8_t)a,(uint8_t)(a>>8)},y[2]={(uint8_t)b,(uint8_t)(b>>8)};
 CHECK("distinct equal hashes",a!=b && tiny_hash(x,2)==tiny_hash(y,2),"");
 CHECK("NULL outputs",first_collision(NULL,&b)==0 && first_collision(&a,NULL)==0,"");return 0;}''', hints=('Track a `uint32_t h`; truncation to uint8_t at return keeps the low eight bits.', 'Initialize 256 bucket slots to -1. Stop on the first bucket already containing an earlier v.'))

    lesson(track,'09-constant-time','Compare every byte',
           'Replace an early-exit equality check with a full scan and understand what source-level timing discipline can prove.',
           '''An ordinary string comparison stops at the first mismatch. For a secret token, repeated measurements can reveal how much of a guessed prefix matched. A useful first repair is to examine every byte and accumulate differences without branching on their values.

Compute `diff |= a[i] ^ b[i]` for every index, then compare diff to zero once. There is no need to count the differing bytes: any nonzero accumulated difference means the inputs differ. Inputs here have a public, fixed length; branching on that length or on invalid pointers is outside the secret-dependent scan.

The exercise exposes an optional `visited` output so tests can check the intended loop work. This is an educational accounting aid, not a timing measurement or proof. Compilers, machine instructions, caches and the surrounding protocol can all affect timing. In production use the vetted library's constant-time comparison function and its length contract. Do not invent a delay, random sleep or hand-tuned loop and call it secure. Constant-time discipline concerns the whole path that handles the secret.''',
           'Implement `int equal_bytes(const uint8_t *a,const uint8_t *b,size_t n,size_t *visited)`. For valid inputs inspect all n byte pairs, return 1 iff equal, and set *visited to n if supplied. Zero length is equal, including NULL pointers. If n>0 and either pointer is NULL, return 0 and set visited to 0. Use XOR and OR accumulation with no early return on a differing byte.',
           '''int equal_bytes(const uint8_t *a,const uint8_t *b,size_t n,size_t *visited){
 if(visited)*visited=0;if(n && (!a||!b))return 0;
 unsigned diff=0;for(size_t i=0;i<n;i++){diff|=a[i]^b[i];if(visited)(*visited)++;}return diff==0;
}
int main(void){return 0;}''',
           '''int equal_bytes(const uint8_t *a,const uint8_t *b,size_t n,size_t *visited){
 if(visited)*visited=0;if(n && (!a||!b))return 0;
 for(size_t i=0;i<n;i++){if(visited)(*visited)++;if(a[i]!=b[i])return 0;}return 1;
}
int main(void){return 0;}''',
           '''int main(void){uint8_t a[64],b[64];for(size_t i=0;i<64;i++)a[i]=(uint8_t)(i*17);
 for(size_t n=1;n<=64;n*=2){memcpy(b,a,n);size_t v=99;CHECK("equal full scan",equal_bytes(a,b,n,&v)==1 && v==n,"");
 for(size_t k=0;k<n;k++){memcpy(b,a,n);b[k]^=0x80;int rc=equal_bytes(a,b,n,&v);CHECK("mismatch full scan",rc==0 && v==n,"n=%zu position=%zu visited=%zu",n,k,v);}}
 size_t v=1;CHECK("empty",equal_bytes(NULL,NULL,0,&v)==1 && v==0,"");
 CHECK("invalid",equal_bytes(NULL,a,3,&v)==0 && v==0,"");CHECK("optional count",equal_bytes(a,a,64,NULL)==1,"");return 0;}''', hints=('An early return on `a[i] != b[i]` reveals the first mismatch position.', 'OR all byte XORs into one unsigned accumulator; decide equality only after the loop.'))

    lesson(track,'10-review-token-bytes','Review: token bytes',
           'Combine strict hex decoding and a complete byte comparison while keeping encoding separate from authentication.',
           '''This review combines [hex encoding](/learn/sec-crypto/hex) with [full-scan equality](/learn/sec-crypto/constant-time). A presented token is lowercase or uppercase hexadecimal text. The expected token is a byte span. Decode exactly two digits per expected byte, reject invalid text, and compare the decoded bytes.

Encoding changes representation; it does not add secrecy or authenticity. Comparing an unpredictable secret token can be one component of authentication, but this toy function generates no secrets and does not implement session storage, replay protection or rate limits. Those concerns remain separate.

Length and malformed-input checks can return early because the syntax and expected length are public in this contract. Once valid bytes are available, accumulate XOR differences across the entire expected length. Check `n <= SIZE_MAX/2` before calculating a doubled text length. The helper accepts empty tokens for the exercise; an application should impose its own nonempty fixed-length policy. No new cryptographic primitive is introduced here.''',
           'Implement `int token_equal(const char *hex,const uint8_t *expected,size_t n)`. Accept exactly 2*n hex digits (either case), no separators, and return 1 iff they decode to expected. Reject NULL hex, NULL expected with n>0, invalid digits or wrong length. Empty string matches n=0. Accumulate differences across all decoded bytes after syntax validation.',
           '''static int digit(char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1;}
int token_equal(const char *hex,const uint8_t *expected,size_t n){
 if(!hex || (n&&!expected) || n>SIZE_MAX/2 || strlen(hex)!=2*n)return 0;
 for(size_t i=0;i<2*n;i++)if(digit(hex[i])<0)return 0;
 unsigned diff=0;for(size_t i=0;i<n;i++)diff|=((digit(hex[2*i])<<4)|digit(hex[2*i+1]))^expected[i];return diff==0;
}
int main(void){return 0;}''',
           '''int token_equal(const char *hex,const uint8_t *expected,size_t n){(void)hex;(void)expected;(void)n;return 0;}
int main(void){return 0;}''',
           '''int main(void){uint8_t e[]={0,0xab,255,0x10};
 const char *s[]={"00abff10","00ABFF10","00abff11","01abff10","00abff1","00abff1000","00agff10","00 abff10","00abff1x"};
 for(size_t i=0;i<9;i++)CHECK("strict token bytes",token_equal(s[i],e,4)==(i<2),"case %zu",i);
 CHECK("empty",token_equal("",NULL,0)==1,"");CHECK("NULL",token_equal(NULL,e,4)==0 && token_equal("aa",NULL,1)==0,"");
 for(unsigned v=0;v<256;v++){char h[3];uint8_t b=(uint8_t)v;snprintf(h,sizeof h,"%02x",v);CHECK("all bytes",token_equal(h,&b,1)==1,"byte %u",v);}return 0;}''',hints=('Reuse a hex-digit helper with a -1 error value.', 'Validate the public syntax first, then OR together every decoded byte XOR expected byte.'),review=True)


def forensics():
    track='sec-forensics'
    (ROOT/track/'track.json').write_text(json.dumps(dict(
        title='Digital Forensics in C',summary='Inspect synthetic evidence: file signatures, disk structures, metadata, integrity checks, timelines and authentication logs.',
        description='Work only with deterministic, fictional evidence. Parse bytes without trusting lengths, distinguish observations from conclusions, and correlate timestamps and log events. The file carver and metadata reader deliberately cover small teaching subsets; the FNV integrity exercise is noncryptographic. Real investigations require preservation, validated tools and an authenticated chain of custody.',
        level='Intermediate',language='c',path='security',order=4,prerequisite='c-systems'),indent=2)+'\n')
    identify=r'''const char *identify(const uint8_t *p,size_t n){
 static const struct {const char *name,*sig;size_t n;} s[]={
 {"PNG","\211PNG\r\n\032\n",8},{"JPEG","\377\330\377",3},{"GIF","GIF87a",6},{"GIF","GIF89a",6},
 {"PDF","%PDF-",5},{"ZIP","PK\003\004",4},{"ELF","\177ELF",4}};
 for(size_t i=0;i<sizeof s/sizeof s[0];i++)if(n>=s[i].n && !memcmp(p,s[i].sig,s[i].n))return s[i].name;
 return "unknown";
}'''
    fnv='''uint64_t fnv1a64(const uint8_t *p,size_t n){uint64_t h=0xcbf29ce484222325ULL;for(size_t i=0;i<n;i++)h=(h^p[i])*0x100000001b3ULL;return h;}'''
    lesson(track,'01-file-signatures','File signatures',
           'Identify a small set of file formats from complete magic bytes at offset zero.',
           '''A filename is a label supplied by someone. A file's first bytes can offer stronger evidence of its format. PNG begins with eight fixed bytes, including nonprintable values; JPEG begins `ff d8 ff`. These are often called magic bytes or signatures.

Compare bytes, not C strings: a signature may contain zeros, and evidence is supplied with an explicit length. Check `n >= signature_length` before `memcmp`. This lesson recognizes PNG (`89 50 4e 47 0d 0a 1a 0a`), JPEG (`ff d8 ff`), GIF87a/GIF89a, PDF (`%PDF-`), ZIP local headers (`50 4b 03 04`) and ELF (`7f 45 4c 46`). Match only at offset zero and return `unknown` for everything else.

A signature is an observation, not validation of the complete format. A truncated file can start with the right bytes, an attacker can forge them, and an empty ZIP uses a different marker from the local-file marker covered here. State precisely what the detector supports. All evidence in this course is fictional and generated by `scripts/content-gen/sec-forensics.py` with fixed seeds; no personal data or real disk image is used.''',
           'Implement `const char *identify(const uint8_t *p,size_t n)` for the signatures listed in the lesson. Return exactly PNG, JPEG, GIF, PDF, ZIP, ELF, or unknown. An incomplete signature and a signature at a nonzero offset do not match. p points to n readable bytes; n may be zero.',
           identify+'\nint main(void){return 0;}',
           'const char *identify(const uint8_t *p,size_t n){(void)p;(void)n;return "unknown";}\nint main(void){return 0;}',
           hints=('Make a table of signature, byte count and name.', 'Before `memcmp`, prove the whole signature lies within n bytes.'),difficulty='easy')

    lesson(track,'02-hex-dump','A byte-level view',
           'Produce an xxd-style display with fixed offsets, paired hex bytes and a printable ASCII column.',
           '''Hex dumps let an analyst check what a parser claims against the underlying bytes. Each row begins with an eight-digit hexadecimal byte offset and displays at most sixteen bytes. Pair the hex bytes in groups of two, separated by one space. Pad that hex column to 39 characters, then add two spaces and a printable view.

Only bytes 0x20 through 0x7e are printable here; replace every other value with a dot. This deliberately uses an ASCII rule rather than the process locale. A final short row still aligns its ASCII column with the full rows. Empty evidence produces an empty string.

Build a row in a temporary buffer, measuring the actual number of characters, then copy only what fits into the caller's output. The return value is the full required output length, like `snprintf`, so a caller can detect truncation. Preserve room for the terminator whenever cap is positive. This display format is a teaching tool, not a file parser; it makes offsets, embedded zeros and truncation visible without interpreting the evidence.''',
           'Implement `size_t hexdump(const uint8_t *p,size_t n,char *out,size_t cap)`. Rows are `%08zx: `, hex pairs separated by spaces in a 39-character padded field, two spaces, ASCII/dots, newline. Example one-byte row: `00000000: 41                                       A\n`. Return the full required length, truncate safely, and terminate for cap>0. For cap=0 out may be NULL; p is valid for n bytes.',
           r'''size_t hexdump(const uint8_t *p,size_t n,char *out,size_t cap){size_t total=0;
 for(size_t off=0;off<n;off+=16){char row[100],hex[40],asc[17];size_t m=n-off<16?n-off:16,h=0;
  for(size_t i=0;i<m;i++){if(i && i%2==0)hex[h++]=' ';snprintf(hex+h,sizeof hex-h,"%02x",p[off+i]);h+=2;asc[i]=p[off+i]>=32&&p[off+i]<127?(char)p[off+i]:'.';}
  while(h<39)hex[h++]=' ';hex[h]=0;asc[m]=0;int k=snprintf(row,sizeof row,"%08zx: %s  %s\n",off,hex,asc);
  for(int i=0;i<k;i++){if(cap && total<cap-1)out[total]=row[i];total++;}
 }if(cap)out[total<cap?total:cap-1]=0;return total;}
int main(void){return 0;}''',
           'size_t hexdump(const uint8_t *p,size_t n,char *out,size_t cap){(void)p;(void)n;if(cap)out[0]=0;return 0;}\nint main(void){return 0;}',
           hints=('Construct the hex field separately; insert a separator before each even byte index after zero.', 'Pad the field to 39 characters. Accumulate required length independently from the number of characters stored.'))

    lesson(track,'03-carving-files','Carve a synthetic image',
           'Locate complete JPEG-like and PNG-like byte ranges while respecting input and output bounds.',
           '''Carving searches raw bytes for recognizable boundaries when a directory entry is unavailable. In this simplified image, a JPEG-like object starts `ff d8 ff` and ends at the first following `ff d9`. A PNG-like object starts with the full PNG signature and ends eight bytes after the first following ASCII `IEND` marker: four letters plus four CRC bytes.

Scan from low offsets to high offsets. When a complete object is found, report its type, start offset and length, then continue after it so the reported ranges do not overlap. When a header has no complete footer, advance one byte and continue searching. Stop when the output array is full. Never subtract or read past an unvalidated end offset.

These are deliberately synthetic formats. A real PNG parser follows chunk lengths and validates structure/CRCs; a real JPEG parser understands marker escaping and segments. Coincidental markers, fragmented files and embedded objects can confuse a signature-only carver. A recovered range is a candidate artifact, not proof of a usable original file. Keep the original image untouched and record offsets so an analyst can reproduce each finding.''',
           'Keep `typedef struct {const char *type; size_t offset,length;} Carved;`. Implement `size_t carve(const uint8_t *p,size_t n,Carved *out,size_t max)`. Return the number stored, at most max, using the synthetic boundary rules above. Scan in offset order and skip complete recovered ranges. Empty data or max=0 returns 0.',
           r'''typedef struct {const char *type;size_t offset,length;} Carved;
size_t carve(const uint8_t *p,size_t n,Carved *out,size_t max){size_t count=0,i=0;
 while(i<n && count<max){size_t end=0;const char *type=NULL;
  if(n-i>=3 && !memcmp(p+i,"\377\330\377",3)){for(size_t j=i+3;j+1<n;j++)if(p[j]==255&&p[j+1]==217){end=j+2;type="JPEG";break;}}
  else if(n-i>=8 && !memcmp(p+i,"\211PNG\r\n\032\n",8)){for(size_t j=i+8;n-j>=8;j++)if(!memcmp(p+j,"IEND",4)){end=j+8;type="PNG";break;}}
  if(end){out[count++]=(Carved){type,i,end-i};i=end;}else i++;
 }return count;}
int main(void){return 0;}''',
           'typedef struct {const char *type;size_t offset,length;} Carved;\nsize_t carve(const uint8_t *p,size_t n,Carved *out,size_t max){(void)p;(void)n;(void)out;(void)max;return 0;}\nint main(void){return 0;}',
           hints=('Validate the complete header before comparing it.', 'Store only a complete range, then set the scan offset to its end. Check max before writing out[count].'))

    lesson(track,'04-partition-tables','Read an MBR',
           'Decode the four little-endian partition slots in a synthetic 512-byte master boot record.',
           '''The legacy master boot record contains four 16-byte partition slots starting at offset 446 and a `55 aa` signature at offsets 510 and 511. This exercise receives exactly 512 bytes, so the sector length is a caller guarantee. Reject a swapped or missing signature.

Within each slot, byte 0 is the boot indicator, byte 4 the partition type, bytes 8..11 the starting LBA and bytes 12..15 the sector count. The two numbers are little-endian: combine bytes with explicit unsigned shifts. Ignore slots whose type or sector count is zero and compact the others into output order. Bootable means the indicator equals 0x80.

The type byte is only a label. Use the names listed in the task and report unrecognized values as Unknown. A GPT protective MBR does not describe all GPT partitions, an extended partition is not the complete logical-partition chain, and partition bounds are not validated against any actual disk size here. Documenting those limits prevents a small decoder from presenting itself as a complete disk validator.''',
           'Keep `typedef struct {int bootable;uint8_t type;uint32_t start_lba,sectors;} Partition;`. Implement `int parse_mbr(const uint8_t *sector,Partition out[4])`, returning -1 for a bad signature or the number of nonempty slots. Implement `const char *type_name(uint8_t type)`: 07 NTFS/exFAT, 0b FAT32 (CHS), 0c FAT32 (LBA), 82 Linux swap, 83 Linux, 05 Extended, ee GPT protective, ef EFI System, else Unknown.',
           '''typedef struct {int bootable;uint8_t type;uint32_t start_lba,sectors;} Partition;
static uint32_t le32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
int parse_mbr(const uint8_t *p,Partition out[4]){if(p[510]!=0x55||p[511]!=0xaa)return -1;int count=0;
 for(int i=0;i<4;i++){const uint8_t *e=p+446+16*i;uint32_t sectors=le32(e+12);if(e[4]&&sectors)out[count++]=(Partition){e[0]==0x80,e[4],le32(e+8),sectors};}return count;}
const char *type_name(uint8_t t){switch(t){case 7:return "NTFS/exFAT";case 11:return "FAT32 (CHS)";case 12:return "FAT32 (LBA)";case 0x82:return "Linux swap";case 0x83:return "Linux";case 5:return "Extended";case 0xee:return "GPT protective";case 0xef:return "EFI System";default:return "Unknown";}}
int main(void){return 0;}''',
           '''typedef struct {int bootable;uint8_t type;uint32_t start_lba,sectors;} Partition;
int parse_mbr(const uint8_t *p,Partition out[4]){(void)p;(void)out;return -1;}
const char *type_name(uint8_t t){(void)t;return "Unknown";}
int main(void){return 0;}''',
           hints=('The four slot addresses are `sector + 446 + 16*i`.', 'Decode each 32-bit value by casting bytes to uint32_t before shifting.'))

    lesson(track,'05-fat-directory','Recover short directory entries',
           'Decode synthetic FAT short-name directory records and mark deleted entries without inventing lost names.',
           '''A FAT short-name directory uses 32-byte records. Byte zero is special: 0x00 ends the directory, while 0xe5 marks a deleted entry. Deleted entries may still contain useful size and cluster fields, but the first character of the original name is lost. Represent it as `_` rather than guessing.

Bytes 0..7 contain a space-padded base name, bytes 8..10 a space-padded extension and byte 11 attributes. Skip long-file-name entries (attribute 0x0f) and volume labels (bit 0x08); retain directories and mark them using bit 0x10. Trim trailing spaces from each name part and insert a dot only when an extension remains.

Combine the high cluster word at offset 20 and low word at offset 26, then read the 32-bit size at offset 28, all little-endian. Process only complete records and stop once the output array is full. This lesson lists metadata only; it does not traverse allocation chains or recover contents. A deleted entry's old clusters might already belong to another file, so its metadata alone cannot establish what was recovered.''',
           'Keep `DirEntry` as supplied. Implement `int read_dir(const uint8_t *p,size_t n,DirEntry *out,size_t max)`. Apply the record rules above, retain deleted entries with _ as first character, and return the number stored (at most max). Ignore incomplete trailing bytes; stop at first 0x00 record. For this subset names are ASCII, and first-byte 0x05 escaping is not used.',
           '''typedef struct {char name[13];int deleted,is_dir;uint32_t size,cluster;} DirEntry;
static uint32_t le32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static unsigned le16(const uint8_t *p){return p[0]|(unsigned)p[1]<<8;}
int read_dir(const uint8_t *p,size_t n,DirEntry *out,size_t max){size_t count=0;
 for(size_t off=0;n-off>=32 && count<max;off+=32){const uint8_t *e=p+off;if(!e[0])break;if(e[11]==15 || (e[11]&8))continue;
  DirEntry v={0};v.deleted=e[0]==0xe5;v.is_dir=(e[11]&16)!=0;size_t a=8,b=3;while(a&&e[a-1]==' ')a--;while(b&&e[8+b-1]==' ')b--;
  size_t k=0;for(size_t j=0;j<a;j++)v.name[k++]=j==0&&v.deleted?'_':(char)e[j];if(b)v.name[k++]='.';for(size_t j=0;j<b;j++)v.name[k++]=(char)e[8+j];v.name[k]=0;
  v.cluster=(uint32_t)le16(e+20)<<16|le16(e+26);v.size=le32(e+28);out[count++]=v;
 }return (int)count;}
int main(void){return 0;}''',
           '''typedef struct {char name[13];int deleted,is_dir;uint32_t size,cluster;} DirEntry;
int read_dir(const uint8_t *p,size_t n,DirEntry *out,size_t max){(void)p;(void)n;(void)out;(void)max;return 0;}
int main(void){return 0;}''',hints=('Stop at byte zero 0x00, but keep 0xe5 records and replace the first name character with _.', 'Skip label/LFN records before decoding. The high and low cluster words are separated in the record.'))

    item='typedef struct {const char *name;const uint8_t *data;size_t n;} Item;'
    lesson(track,'06-hash-verification','Compare an evidence manifest',
           'Use a toy checksum manifest to distinguish matching, changed and unlisted evidence items.',
           '''A manifest records a name and digest for each evidence item. Recomputing the digest can reveal a changed copy, but only if the reference manifest itself is trustworthy. In real investigations preserve the original, authenticate the reference and document custody. This exercise uses FNV-1a only because its arithmetic is small enough to inspect; FNV is not collision resistant or suitable for adversarial forensic integrity.

The 64-bit teaching checksum starts at `0xcbf29ce484222325`. For each byte, XOR it into the state and multiply by `0x100000001b3` modulo 2^64. Unsigned `uint64_t` wraparound is intentional. An empty item has the initial state as its digest.

Manifest lines contain sixteen hex digits, two spaces and a filename without spaces. Match full filenames, independently of line order. For each supplied item record status 0 if its digest matches, 1 if the name exists but the digest differs, or 2 if the name is absent. Report only supplied items; detecting missing evidence requires a separate manifest-to-inventory pass and is outside this function's contract.''',
           'Implement `uint64_t fnv1a64(const uint8_t *p,size_t n)` and `int audit(const char *manifest,const Item *items,int n,int *status)`. Set one status per item (0 match, 1 changed, 2 unlisted) and return how many statuses are nonzero. Manifest lines are valid, unique, at most 255 characters, and filenames have no spaces. n is nonnegative. Keep the Item type supplied.',
           fnv+'\n'+item+'''
int audit(const char *manifest,const Item *items,int n,int *status){int bad=0;
 for(int i=0;i<n;i++){status[i]=2;const char *p=manifest;
  while(*p){unsigned long long h;char name[256];if(sscanf(p,"%llx %255s",&h,name)==2 && !strcmp(name,items[i].name)){status[i]=fnv1a64(items[i].data,items[i].n)==(uint64_t)h?0:1;break;}const char *e=strchr(p,'\\n');if(!e)break;p=e+1;}
  if(status[i])bad++;
 }return bad;}
int main(void){return 0;}''',
           item+'\nuint64_t fnv1a64(const uint8_t *p,size_t n){(void)p;(void)n;return 0;}\nint audit(const char *manifest,const Item *items,int n,int *status){(void)manifest;(void)items;for(int i=0;i<n;i++)status[i]=2;return n;}\nint main(void){return 0;}',
           hints=('Keep the state in uint64_t so wraparound is defined.', 'For each item scan manifest lines for an exact name match, then compare the recomputed checksum.'))

    lesson(track,'07-review-evidence-triage','Review: evidence triage',
           'Combine signatures, a short hex preview and a toy checksum into a reproducible evidence observation.',
           '''This review combines [signatures](/learn/sec-forensics/file-signatures), [hex display](/learn/sec-forensics/hex-dump) and [manifest checks](/learn/sec-forensics/hash-verification). No new file format is introduced. Produce one compact observation for each item without relying on its extension.

Use only PNG, JPEG, PDF and ZIP signatures in this review; other formats are unknown. Display at most the first eight bytes, lowercase hex separated by single spaces, or a dash when there are no bytes. Calculate the same 64-bit FNV checksum as before and compare it with the supplied reference.

Report OK or CHANGED relative to that reference. Do not turn OK into a claim of authenticity: this is still the deliberately noncryptographic teaching checksum. Likewise, recognizing a signature does not prove a complete valid file. Keeping those observations separate makes the report reproducible and keeps conclusions proportional to the evidence. The output is bounded and always terminated for a positive capacity, so even a reporting path follows the same defensive rules as the parser.''',
           'Implement `void report(const char *name,const uint8_t *p,size_t n,uint64_t expected,char *out,size_t cap)`. Format exactly `name: TYPE, first bytes HEX, hash 16_lowercase_hex_digits, OK\n` (or CHANGED). HEX is up to 8 lowercase byte pairs separated by spaces, or `-`. Types are PNG/JPEG/PDF/ZIP/unknown. Truncate safely and terminate if cap>0. Inputs and name are valid; out may be NULL only for cap=0.',
           identify+'\n'+fnv+r'''
void report(const char *name,const uint8_t *p,size_t n,uint64_t expected,char *out,size_t cap){char preview[24];size_t k=0;
 if(!n){strcpy(preview,"-");}else{for(size_t i=0;i<n&&i<8;i++){if(i)preview[k++]=' ';snprintf(preview+k,sizeof preview-k,"%02x",p[i]);k+=2;}}
 const char *type=identify(p,n);if(!strcmp(type,"ELF")||!strcmp(type,"GIF"))type="unknown";uint64_t h=fnv1a64(p,n);
 snprintf(out,cap,"%s: %s, first bytes %s, hash %016llx, %s\n",name,type,preview,(unsigned long long)h,h==expected?"OK":"CHANGED");}
int main(void){return 0;}''',
           'void report(const char *name,const uint8_t *p,size_t n,uint64_t expected,char *out,size_t cap){(void)name;(void)p;(void)n;(void)expected;if(cap)out[0]=0;}\nint main(void){return 0;}',
           hints=('Reuse earlier byte-level helpers; limit the preview to eight bytes.', 'Use `%016llx` with a cast to unsigned long long, and select OK only on exact checksum equality.'),review=True)

    meta='typedef struct {char make[64],model[64];uint32_t width,height;char datetime[32];} Meta;'
    date=r'''static int leap(int y){return y%4==0 && (y%100!=0 || y%400==0);}
long long to_unix(const char *s){
 if(!s || strlen(s)!=19)return -1;
 for(int i=0;i<19;i++){if(i==4||i==7||i==13||i==16){if(s[i]!=':')return -1;}else if(i==10){if(s[i]!=' ')return -1;}else if(s[i]<'0'||s[i]>'9')return -1;}
 int y,mo,d,h,mi,se;if(sscanf(s,"%4d:%2d:%2d %2d:%2d:%2d",&y,&mo,&d,&h,&mi,&se)!=6)return -1;
 const int days[]={31,28,31,30,31,30,31,31,30,31,30,31};if(y<1970||y>2100||mo<1||mo>12||d<1||d>days[mo-1]+(mo==2&&leap(y))||h>23||mi>59||se>59)return -1;
 long long total=0;for(int yr=1970;yr<y;yr++)total+=365+leap(yr);for(int m=1;m<mo;m++)total+=days[m-1]+(m==2&&leap(y));total+=d-1;return ((total*24+h)*60+mi)*60+se;
}'''
    lesson(track,'08-photo-metadata','Read synthetic TIFF metadata',
           'Decode a bounded TIFF-style directory in either byte order and normalize fictional timestamps to UTC.',
           '''Photo metadata is structured evidence, not a trustworthy account of when or where an image was created. Editors can alter tags and camera clocks can be wrong. This lesson reads standalone synthetic TIFF-style data, not an entire JPEG/EXIF container.

The header is II for little-endian or MM for big-endian, a 16-bit value 42, and a 32-bit offset to a directory. At that offset is a 16-bit entry count followed by 12-byte entries: tag (16 bits), type (16), count (32) and value/offset (32). Validate the directory range before traversing it. Supported tags are Make 0x010f, Model 0x0110, width 0x0100, height 0x0101 and timestamp 0x0132. Numeric values are one SHORT (type 3) or LONG (4); ASCII is type 2 with a terminating zero included in count. Up to four ASCII bytes are inline; longer strings use an offset. Unknown tags/types are ignored.

Validate each accessed range using subtraction after checking the offset, and cap copies into the result fields. Missing tags remain empty/zero. Timestamps use `YYYY:MM:DD HH:MM:SS`. For this fictional dataset we explicitly assume UTC; real EXIF timestamps often omit a zone, so do not silently apply that assumption to real evidence. Validate Gregorian dates before converting days since 1970 to seconds.''',
           'Keep Meta as supplied. Implement `int read_meta(const uint8_t *p,size_t n,Meta *out)`: return 0 for the supported TIFF subset, -1 for a malformed header/directory/supported value; clear missing fields. Strings must fit (including terminator), numeric tags require count=1; unknown tags/types are ignored. Implement `long long to_unix(const char *s)` for valid Gregorian timestamps in years 1970..2100, interpreting UTC and returning -1 for invalid text/date.',
           meta+r'''
static uint32_t word(const uint8_t *p,int b,int big){uint32_t v=0;if(big){for(int i=0;i<b;i++)v=(v<<8)|p[i];}else{for(int i=b-1;i>=0;i--)v=(v<<8)|p[i];}return v;}
int read_meta(const uint8_t *p,size_t n,Meta *out){if(n<8 || !out)return -1;int big=p[0]=='M'&&p[1]=='M';if(!big && !(p[0]=='I'&&p[1]=='I'))return -1;
 if(word(p+2,2,big)!=42)return -1;size_t off=word(p+4,4,big);if(off>n || n-off<2)return -1;size_t count=word(p+off,2,big);off+=2;if(count>(n-off)/12)return -1;Meta m={0};
 for(size_t i=0;i<count;i++){const uint8_t *e=p+off+12*i;unsigned tag=word(e,2,big),type=word(e+2,2,big);size_t c=word(e+4,4,big);uint32_t v=word(e+8,4,big);char *dst=NULL;size_t cap=0;
  if(tag==0x10f){dst=m.make;cap=sizeof m.make;}else if(tag==0x110){dst=m.model;cap=sizeof m.model;}else if(tag==0x132){dst=m.datetime;cap=sizeof m.datetime;}
  if(dst && type==2){if(!c || c>cap)return -1;const uint8_t *s=e+8;if(c>4){if(v>n||c>n-v)return -1;s=p+v;}if(s[c-1]!=0 || memchr(s,0,c)==NULL)return -1;memcpy(dst,s,c);}
  if((tag==0x100||tag==0x101)&&(type==3||type==4)){if(c!=1)return -1;uint32_t val=type==3?word(e+8,2,big):v;if(tag==0x100)m.width=val;else m.height=val;}
 }*out=m;return 0;}
'''+date+'\nint main(void){return 0;}',
           meta+'\nint read_meta(const uint8_t *p,size_t n,Meta *out){(void)p;(void)n;(void)out;return -1;}\nlong long to_unix(const char *s){(void)s;return -1;}\nint main(void){return 0;}',
           hints=('Write one byte-order helper that reads two or four bytes without unaligned pointer casts.', 'Validate the directory and every referenced ASCII range before use. Count days using Gregorian leap-year rules.'))

    lesson(track,'09-building-a-timeline','Build a stable timeline',
           'Merge fictional timestamped events into UTC order while preserving ties and reporting quiet intervals.',
           '''A timeline puts observations from different sources on one clock. It can expose a sequence that no single log makes clear, but clock drift, missing records and timezone assumptions still limit the conclusion. Here timestamps are already integer Unix seconds and all displayed times are UTC.

Each input line is `timestamp source message`, with the message continuing to the end of the line. Preserve spaces in that message. Sort by timestamp using a stable method: two events at the same second must retain their input order. An insertion sort is adequate for the bounded teaching dataset, or attach an original index as a tie-breaker.

Before each event after the first, compare it with the previous event. A gap of at least 3600 seconds gets a quiet-period line in whole hours and remaining whole minutes. Finally report the event count and the total span from earliest to latest. A quiet gap means only that these supplied records contain no event; it does not establish that nothing happened. The generator deliberately mixes sources and inserts ties so hardcoded output and unstable ordering both fail.''',
           'Read stdin (1..1000 lines, each at most 511 characters) in the format `UnixSeconds source message`. Sources have no spaces; seconds are nonnegative and fit time_t. Sort stably by timestamp. Print `YYYY-MM-DDTHH:MM:SSZ [source] message`. Before gaps >=3600s print `-- quiet for Hh MMm --`. Finish `N events over Hh MMm`. Minutes are zero-padded, hours are not. No blank lines. The supplied dataset always has valid lines.',
           r'''typedef struct {long long t;char src[64],msg[512];} Event;
int main(void){Event ev[1000];size_t n=0;char line[512];
 while(n<1000 && fgets(line,sizeof line,stdin)){int at=0;if(sscanf(line,"%lld %63s %n",&ev[n].t,ev[n].src,&at)!=2)continue;strcpy(ev[n].msg,line+at);ev[n].msg[strcspn(ev[n].msg,"\r\n")]=0;n++;}
 for(size_t i=1;i<n;i++){Event v=ev[i];size_t j=i;while(j && ev[j-1].t>v.t){ev[j]=ev[j-1];j--;}ev[j]=v;}
 for(size_t i=0;i<n;i++){if(i && ev[i].t-ev[i-1].t>=3600){long long g=ev[i].t-ev[i-1].t;printf("-- quiet for %lldh %02lldm --\n",g/3600,g%3600/60);}time_t t=(time_t)ev[i].t;struct tm tm=*gmtime(&t);char out[32];strftime(out,sizeof out,"%Y-%m-%dT%H:%M:%SZ",&tm);printf("%s [%s] %s\n",out,ev[i].src,ev[i].msg);}
 long long span=n?ev[n-1].t-ev[0].t:0;printf("%zu events over %lldh %02lldm\n",n,span/3600,span%3600/60);return 0;}''',
           'int main(void){puts("0 events over 0h 00m");return 0;}',
           hints=('Use `%n` after the source in sscanf to locate the start of the remaining message.', 'Insertion sort shifts only strictly greater timestamps, preserving ties. Use gmtime for UTC.'),mode='stdout')

    lesson(track,'10-auth-log-analysis','Correlate authentication failures',
           'Summarize password failures by source and flag the first later password acceptance after a threshold.',
           '''Authentication logs contain both useful events and noise. Parse the event grammar rather than counting every occurrence of the word Failed. This lesson considers only sshd password failures and password acceptances. Public-key events, connection notices and unrelated programs are ignored.

For a failure, extract the username after `Failed password for`, skipping an optional `invalid user`, and the address after `from`. Maintain a total and a set of distinct usernames for each source address. Mark an address as a candidate when it reaches five failures in this supplied file. If a password acceptance subsequently appears for that address, record the first accepted username. An acceptance before the fifth failure does not qualify.

Sort candidates by descending failure count, then lexicographic address text for ties. The text order is deliberate, not numeric IP order. This is a simple triage rule over a bounded file, with no sliding time window and no assertion that an address identifies a person. NAT, legitimate mistakes and forged/missing logs can all affect interpretation. The word ACCEPTED reports what the synthetic log says; it does not prove that the failures caused the later login.''',
           'Read stdin (at most 1000 lines, 256 source IPs, 256 unique usernames per IP; names/IPs at most 63 characters). Count only sshd `Failed password for [invalid user ]USER from IP ...`. For IPs with >=5 failures print `IP failures=N users=U`, appending ` ACCEPTED as USER` for the first sshd password acceptance after that IP reached five failures. Ignore publickey and other events. Sort by failures descending, then strcmp(IP). If none qualify print `no brute force detected`. One newline per row.',
           r'''typedef struct {char ip[64],users[256][64],accepted[64];int fails,count;} Source;
static Source src[256];
int main(void){char line[1024];int n=0;
 while(fgets(line,sizeof line,stdin)){if(!strstr(line,"sshd["))continue;char *p=strstr(line," Failed password for ");int failed=p!=NULL;if(p)p+=21;else{p=strstr(line," Accepted password for ");if(!p)continue;p+=23;}
  if(failed && !strncmp(p,"invalid user ",13))p+=13;char user[64],ip[64];if(sscanf(p,"%63s from %63s",user,ip)!=2)continue;int j=0;while(j<n && strcmp(src[j].ip,ip))j++;if(j==n){if(n==256)continue;strcpy(src[n++].ip,ip);}
  if(failed){src[j].fails++;int k=0;while(k<src[j].count && strcmp(src[j].users[k],user))k++;if(k==src[j].count && k<256)strcpy(src[j].users[src[j].count++],user);}
  else if(src[j].fails>=5 && !src[j].accepted[0])strcpy(src[j].accepted,user);
 }
 int order[256],m=0;for(int i=0;i<n;i++)if(src[i].fails>=5)order[m++]=i;
 for(int i=1;i<m;i++){int v=order[i],j=i;while(j && (src[order[j-1]].fails<src[v].fails || (src[order[j-1]].fails==src[v].fails && strcmp(src[order[j-1]].ip,src[v].ip)>0))){order[j]=order[j-1];j--;}order[j]=v;}
 if(!m)puts("no brute force detected");for(int i=0;i<m;i++){Source *s=&src[order[i]];printf("%s failures=%d users=%d",s->ip,s->fails,s->count);if(s->accepted[0])printf(" ACCEPTED as %s",s->accepted);putchar('\n');}return 0;}''',
           'int main(void){puts("no brute force detected");return 0;}',
           hints=('Skip the literal `invalid user ` before parsing the username; use exact password event prefixes.', 'Track acceptance during the input scan so an earlier login cannot be attached after later failures.'),mode='stdout')


if __name__ == '__main__':
    bugs()
    crypto()
    forensics()
