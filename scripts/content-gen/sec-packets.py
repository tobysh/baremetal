#!/usr/bin/env python3
"""Generate deterministic, entirely synthetic packet lessons and pcap evidence.

No capture interface, sockets, subprocesses, third-party modules or real hosts.
Run: python3 scripts/content-gen/sec-packets.py
Expected values are constructed in Python, independently of the C solutions.
"""
import json
import random
import runpy
import struct
from pathlib import Path

BASE = Path(__file__).resolve().parent
author = runpy.run_path(str(BASE / 'sec-lessons.py'))['lesson']
ROOT = BASE.parents[1] / 'content' / 'sec-packets'
RNG = random.Random(20261029)
TRACK = 'sec-packets'
BE = '''static uint16_t be16(const uint8_t *p){return (uint16_t)((unsigned)p[0]<<8|p[1]);}
static uint32_t be32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
'''


def arr(name, data):
    return 'static const uint8_t %s[]={%s};\n' % (name, ','.join(str(x) for x in data) or '0')


def cstr(s):
    return json.dumps(s)


def samples(name, values):
    return ''.join(arr(name+str(i), value) for i, value in enumerate(values))


def rows(name, values, expected, fields):
    return ('static const struct {const uint8_t *p;size_t n;' + fields + '} V[]={\n' +
            ',\n'.join('{%s%d,%d,%s}' % (name,i,len(b),e) for i,(b,e) in enumerate(zip(values,expected)))+'\n};\n')


def teach(folder,title,summary,body,task,solution,starter,tests,hints,review=False,difficulty='medium'):
    author(TRACK,folder,title,summary,body,task,solution+'\nint main(void){return 0;}',
           starter+'\nint main(void){return 0;}',tests,hints=hints,review=review,difficulty=difficulty)


def eth(payload, kind=0x0800, vlan=None):
    header=bytes.fromhex('020000000002020000000001')
    return header+(struct.pack('!HHH',0x8100,vlan,kind) if vlan is not None else struct.pack('!H',kind))+payload


def ipv4(payload, proto, src=0xc000020a, dst=0xc6336414, options=b'', frag=0):
    h=bytearray(struct.pack('!BBHHHBBHII',0x45+len(options)//4,0,20+len(options)+len(payload),123,frag,64,proto,0,src,dst)+options)
    s=sum(struct.unpack('!%dH'%(len(h)//2),h))
    while s>>16:s=(s&65535)+(s>>16)
    h[10:12]=struct.pack('!H',s^65535)
    return bytes(h)+payload


def tcp(payload=b'',sport=40000,dport=80,seq=1000,ack=0,flags=2,options=b''):
    return struct.pack('!HHIIBBHHH',sport,dport,seq,ack,(5+len(options)//4)<<4,flags,4096,0,0)+options+payload


def udp(payload=b'',sport=40001,dport=53):
    return struct.pack('!HHHH',sport,dport,8+len(payload),0)+payload


def dns(name, ident=0x1234,qtype=1,qclass=1):
    labels=b''.join(bytes([len(x)])+x.encode() for x in name.split('.')) if name else b''
    return struct.pack('!6H',ident,0x0100,1,0,0,0)+labels+b'\0'+struct.pack('!HH',qtype,qclass)


def arp(op=1,src=0xc000020a,dst=0xc0000201):
    return struct.pack('!HHBBH6sI6sI',1,0x0800,6,4,op,bytes.fromhex('020000000001'),src,b'\0'*6,dst)


def pcap(frames,big=False):
    order='>' if big else '<'
    out=struct.pack(order+'IHHIIII',0xa1b2c3d4,2,4,0,0,65535,1)
    for i,f in enumerate(frames):
        out+=struct.pack(order+'IIII',1793232000+i,100*i,len(f),len(f))+f
    return out


ETH_TYPE='typedef struct {uint8_t dst[6],src[6];uint16_t type;size_t payload;} Ethernet;'
ETH_CODE=BE+ETH_TYPE+'''
int parse_ethernet(const uint8_t *p,size_t n,Ethernet *out){
 if(!p||!out||n<14)return -1;Ethernet e={0};memcpy(e.dst,p,6);memcpy(e.src,p+6,6);e.type=be16(p+12);e.payload=14;
 if(e.type==0x8100){if(n<18)return -1;e.type=be16(p+16);e.payload=18;}*out=e;return 0;
}'''
IP_TYPE='typedef struct {uint32_t src,dst;uint16_t total;uint8_t protocol;size_t header;int fragmented;} IPv4;'
IP_FUNC='''int parse_ipv4(const uint8_t *p,size_t n,IPv4 *out){
 if(!p||!out||n<20 || p[0]>>4!=4)return -1;size_t h=(p[0]&15)*4;uint16_t total=be16(p+2);
 if(h<20 || h>n || total<h || total>n)return -1;
 *out=(IPv4){be32(p+12),be32(p+16),total,p[9],h,(be16(p+6)&0x3fff)!=0};return 0;
}'''
TCP_TYPE='typedef struct {uint16_t src,dst;uint32_t seq,ack;uint8_t flags;size_t header,payload;} TCP;'
TCP_FUNC='''int parse_tcp(const uint8_t *p,size_t n,TCP *out){
 if(!p||!out||n<20)return -1;size_t h=(p[12]>>4)*4;if(h<20||h>n)return -1;
 *out=(TCP){be16(p),be16(p+2),be32(p+4),be32(p+8),p[13],h,n-h};return 0;
}'''
UDP_TYPE='typedef struct {uint16_t src,dst;size_t payload;} UDP;'
UDP_FUNC='''int parse_udp(const uint8_t *p,size_t n,UDP *out){
 if(!p||!out||n<8)return -1;size_t len=be16(p+4);if(len<8||len>n)return -1;*out=(UDP){be16(p),be16(p+2),len-8};return 0;
}'''


def main():
    ROOT.mkdir(parents=True,exist_ok=True)
    (ROOT/'track.json').write_text(json.dumps(dict(title='Packet Analysis in C',
        summary='Decode synthetic pcap, Ethernet, ARP, IP, TCP, UDP and DNS; rebuild a tiny HTTP stream and review traffic anomalies.',
        description='Inspect offline, deterministic teaching captures using bounds-checked C parsers. Each lesson states its supported protocol subset and equivalent Wireshark display filters. No live capture or network access is involved. Documentation addresses and fictional .test names keep the evidence separate from real traffic.',
        language='c',level='Intermediate',path='security',order=3,prerequisite='c-systems'),indent=2)+'\n')

    # Valid offline Ethernet capture plus malformed/truncated variants for the first lesson.
    frames=[eth(arp(),0x0806),eth(ipv4(udp(dns('notes.example.test')),17)),
            eth(ipv4(tcp(),6)),eth(ipv4(tcp(b'GET /notes HTTP/1.1\r\nHost: example.test\r\n\r\n',flags=0x18),6))]
    fixtures=ROOT/'fixtures';fixtures.mkdir(exist_ok=True)
    (fixtures/'sample.pcap').write_bytes(pcap(frames))
    (fixtures/'sample-big-endian.pcap').write_bytes(pcap(frames,True))
    (fixtures/'README.md').write_text('''# Synthetic packet evidence

Run `python3 scripts/content-gen/sec-packets.py` to reproduce these byte-for-byte.
All timestamps, MACs and payloads are invented. IPs are from documentation ranges
192.0.2.0/24, 198.51.100.0/24 and 203.0.113.0/24. Names use example.test.
There is no live capture, external traffic, personal data or embedded secret.

Open sample.pcap in Wireshark, or run `tshark -r content/sec-packets/fixtures/sample.pcap`.
The big-endian variant contains the same frames and tests capture byte-order handling.
TCP checksums in this teaching fixture are zero (the lesson does not validate them);
UDP zero checksums are permitted for IPv4. IPv4 headers have valid checksums.

Useful display filters (not capture filters):

| Lesson | Wireshark display filter or view |
| --- | --- |
| pcap | frame; inspect Frame captured length / length |
| Ethernet | eth.type == 0x0800; vlan for tagged examples |
| ARP | arp; arp.opcode == 1 |
| IPv4 | ip; ip.flags.mf == 1 || ip.frag_offset > 0 |
| TCP | tcp; tcp.flags.syn == 1 && tcp.flags.ack == 0 |
| UDP | udp; udp.port == 53 |
| DNS | dns; dns.qry.name == "notes.example.test" |
| HTTP stream | tcp.port == 80; Follow > TCP Stream |
| Anomaly | tcp.flags.syn == 1 && tcp.flags.ack == 0; Statistics > Conversations |
| Review | arp || ip |

The anomaly lesson uses synthetic flow summaries in its harness, not additional
real traffic. Malformed parser cases remain embedded in tests rather than mixed
into the valid demonstration capture. Display filters select observations;
they do not establish that a source is malicious.
''')
    caps=[pcap([]),pcap(frames[:1]),pcap(frames),pcap(frames,True)]
    want=['0','1','4','4']
    shorter=bytearray(pcap(frames[:1]));shorter[36:40]=struct.pack('<I',len(frames[0])+100)
    caps.append(bytes(shorter));want.append('1')  # valid capture truncation
    bad=bytearray(caps[2]);bad[0]=0;caps.append(bytes(bad));want.append('-1')
    caps.extend([caps[2][:10],caps[2][:-1],caps[2]+b'\0',caps[2][:30]]);want.extend(['-1']*4)
    for offset,value in [(4,3),(20,2),(24+8,65536),(24+12,1),(16,0),(24+4,1000000)]:
        b=bytearray(pcap(frames[:1]));b[offset:offset+4]=struct.pack('<I',value);caps.append(bytes(b));want.append('-1')
    tests=samples('c',caps)+rows('c',caps,want,'int want;')+'''
int main(void){for(size_t i=0;i<sizeof V/sizeof V[0];i++){int got=pcap_count(V[i].p,V[i].n);CHECK("capture structure",got==V[i].want,"capture %zu expected %d got %d",i,V[i].want,got);}return 0;}'''
    teach('01-pcap','Read a classic pcap container','Count complete pcap records while checking byte order, lengths and the supported link type.',
          '''A capture is a container around frames. Classic pcap starts with a 24-byte global header and stores each captured frame behind a 16-byte record header. It is different from pcapng; this lesson deliberately supports only classic microsecond-resolution pcap, version 2.4, Ethernet link type 1.

The first four bytes select byte order: `d4 c3 b2 a1` means little-endian and `a1 b2 c3 d4` means big-endian. Decode subsequent integers in that order. The version is two 16-bit fields at offsets 4 and 6, snaplen is a 32-bit value at 16, and link type is at 20. Snaplen must be nonzero.

Each record has seconds, microseconds, captured length and original wire length, all 32 bits. Require microseconds below one million, captured length no larger than snaplen or original length, and enough remaining bytes for the captured frame. A shorter capture than wire length is valid truncation; walking beyond the container is not. Validate with subtraction instead of unchecked `offset + length`.

The reproducible files are in `content/sec-packets/fixtures/`; open sample.pcap in Wireshark. Equivalent display filter: `frame`. Compare Frame captured length with Frame length. There is no live capture in this course, and every address and payload is fictional.''',
          'Implement `int pcap_count(const uint8_t *p,size_t n)`. Return the record count or -1 for unsupported/malformed/truncated data, including trailing partial record headers. Apply the header and record rules above. Ignore timezone and sigfigs fields. Zero-record files are valid; NULL input is invalid. Counts in these fixtures fit int.',
          '''static uint32_t readnum(const uint8_t *p,int n,int big){uint32_t v=0;if(big){for(int i=0;i<n;i++)v=(v<<8)|p[i];}else{for(int i=n-1;i>=0;i--)v=(v<<8)|p[i];}return v;}
int pcap_count(const uint8_t *p,size_t n){if(!p||n<24)return -1;int big;
 if(!memcmp(p,"\\xd4\\xc3\\xb2\\xa1",4))big=0;else if(!memcmp(p,"\\xa1\\xb2\\xc3\\xd4",4))big=1;else return -1;
 if(readnum(p+4,2,big)!=2 || readnum(p+6,2,big)!=4 || !readnum(p+16,4,big) || readnum(p+20,4,big)!=1)return -1;
 uint32_t snap=readnum(p+16,4,big);size_t off=24;int count=0;while(off<n){if(n-off<16)return -1;uint32_t micro=readnum(p+off+4,4,big),cap=readnum(p+off+8,4,big),orig=readnum(p+off+12,4,big);off+=16;
 if(micro>=1000000 || cap>snap || cap>orig || cap>n-off)return -1;off+=cap;count++;}return count;}''',
          'int pcap_count(const uint8_t *p,size_t n){(void)p;(void)n;return -1;}',tests,
          ('Recognize byte order from the raw magic bytes before decoding any integers.', 'For every record, prove `n-off >= 16`, then require the captured length to fit the remaining bytes.'))

    vals=[];wants=[]
    for i in range(8):
        vlan=i%2==1;k=[0x0800,0x0806,0x86dd,0x88b5][i//2]
        b=eth(bytes(RNG.randrange(256) for _ in range(i)),k,42 if vlan else None)
        vals.append(b);wants.append('{0,%d,%d}'%(k,18 if vlan else 14))
    vals.extend([b'',eth(b'')[:13],eth(b'',vlan=1)[:17]]);wants.extend(['{-1,0,0}']*3)
    data=samples('e',vals)+rows('e',vals,wants,'struct {int rc;unsigned type;size_t off;} want;')
    tests=data+'''
int main(void){for(size_t i=0;i<sizeof V/sizeof V[0];i++){Ethernet e={0};int rc=parse_ethernet(V[i].p,V[i].n,&e);int ok=rc==V[i].want.rc;
 if(rc==0)ok=ok && e.type==V[i].want.type && e.payload==V[i].want.off && !memcmp(e.dst,V[i].p,6) && !memcmp(e.src,V[i].p+6,6);
 CHECK("Ethernet fields",ok,"frame %zu",i);}return 0;}'''
    teach('02-ethernet','Ethernet boundaries','Decode MAC addresses and EtherType, including one 802.1Q VLAN tag.',
          '''An Ethernet II header starts with six destination MAC bytes, six source MAC bytes and a two-byte EtherType in network (big-endian) order. The ordinary payload begins at byte 14. MAC addresses are byte sequences; do not cast them to machine integers or unaligned structs.

When the EtherType is 0x8100, this subset accepts one 802.1Q tag: bytes 14..15 are the tag control information and bytes 16..17 are the encapsulated EtherType. The payload then starts at 18. Validate the longer header before reading it. Other types, including a second tag marker, are returned as uninterpreted types; stacked VLAN parsing is outside this exercise.

The parser reports boundaries, not whether the payload protocol is valid. Capture files normally omit the Ethernet frame check sequence and may retain padding; neither belongs in the fixed header calculation. Equivalent Wireshark display filters are `eth.type == 0x0800` for IPv4 and `vlan` for tagged frames. The lesson harness includes tags even though the small demonstration capture uses ordinary Ethernet.''',
          'Keep Ethernet as supplied. Implement `int parse_ethernet(const uint8_t *p,size_t n,Ethernet *out)`, returning 0 on success and -1 on a NULL pointer or truncated header. Copy both MAC arrays, decode type, set payload offset 14 or 18 for one 0x8100 tag. Do not change *out on failure.',
          ETH_CODE,ETH_TYPE+'\nint parse_ethernet(const uint8_t *p,size_t n,Ethernet *out){(void)p;(void)n;(void)out;return -1;}',tests,
          ('Decode the two-byte type as `(p[12] << 8) | p[13]`.', 'If it is 0x8100, require 18 bytes before reading the inner type.'),difficulty='easy')

    arp_type='typedef struct {uint16_t opcode;uint32_t sender,target;} ARP;'
    vals=[];wants=[]
    for op,s,d in [(1,0xc000020a,0xc0000201),(2,0xc6336407,0xc6336414),(1,0xcb007109,0xcb0071fe)]:
        vals.append(arp(op,s,d));wants.append('{0,%d,%uu,%uu}'%(op,s,d))
    for off,val in [(1,2),(2,0),(4,5),(5,16),(7,3)]:
        b=bytearray(arp());b[off]=val;vals.append(bytes(b));wants.append('{-1,0,0,0}')
    vals.extend([arp()[:27],b'']);wants.extend(['{-1,0,0,0}']*2)
    tests=samples('a',vals)+rows('a',vals,wants,'struct {int rc;unsigned op;uint32_t src,dst;} want;')+'''
int main(void){for(size_t i=0;i<sizeof V/sizeof V[0];i++){ARP a={0};int rc=parse_arp(V[i].p,V[i].n,&a);CHECK("ARP subset",rc==V[i].want.rc && (rc || (a.opcode==V[i].want.op && a.sender==V[i].want.src && a.target==V[i].want.dst)),"message %zu",i);}return 0;}'''
    teach('03-arp','ARP observations','Read an Ethernet/IPv4 ARP request or reply without treating an address claim as authenticated identity.',
          '''ARP asks which local hardware address is associated with an IPv4 address. Its fields are claims in the packet, not authentication. Duplicate or changing claims can warrant investigation, but one ARP message cannot establish spoofing or the identity of a device.

This lesson accepts Ethernet/IPv4 ARP only: hardware type 1, protocol type 0x0800, hardware length 6 and protocol length 4. A complete message is 28 bytes. Decode the operation at bytes 6..7 (1 request, 2 reply), sender IPv4 bytes 14..17 and target IPv4 bytes 24..27. All integer fields use big-endian order. The Ethernet header was removed by the caller.

Return addresses as canonical numeric values: bytes 192,0,2,10 become 0xc000020a on every host. This differs from interpreting the bytes in native machine order. Ignore trailing link padding. Equivalent Wireshark display filters are `arp` and `arp.opcode == 1`; inspect sender and target protocol addresses. All addresses in these fixtures are reserved for documentation.''',
          'Keep ARP as supplied. Implement `int parse_arp(const uint8_t *p,size_t n,ARP *out)`: return 0 for the complete supported request/reply subset, -1 for NULL, truncation or other header values. Set opcode, sender and target. Do not change *out on failure; extra trailing bytes are allowed.',
          BE+arp_type+'''
int parse_arp(const uint8_t *p,size_t n,ARP *out){if(!p||!out||n<28 || be16(p)!=1 || be16(p+2)!=0x0800 || p[4]!=6 || p[5]!=4)return -1;
 uint16_t op=be16(p+6);if(op!=1&&op!=2)return -1;*out=(ARP){op,be32(p+14),be32(p+24)};return 0;}''',
          arp_type+'\nint parse_arp(const uint8_t *p,size_t n,ARP *out){(void)p;(void)n;(void)out;return -1;}',tests,
          ('Validate all four ARP format fields before using the fixed offsets.', 'Cast each address byte to uint32_t before shifting by 24, 16 or 8.'),difficulty='easy')

    vals=[];wants=[]
    for proto,opts,frag in [(6,b'',0),(17,b'\1'*4,0x4000),(1,b'\1'*12,0x2000),(6,b'',3)]:
        b=ipv4(b'data',proto,options=opts,frag=frag);vals.append(b);wants.append('{0,3221225994u,3325256724u,%d,%d,%d,%d}'%(len(b),proto,20+len(opts),bool(frag&0x3fff)))
    b=ipv4(b'ab',17);vals.append(b+b'padding');wants.append('{0,3221225994u,3325256724u,22,17,20,0}')
    for offset,value in [(0,0x65),(0,0x44),(2,0xff),(3,19),(0,0x4f)]:
        b=bytearray(ipv4(b'',6));b[offset]=value;vals.append(bytes(b));wants.append('{-1,0,0,0,0,0,0}')
    vals.extend([b'',ipv4(b'',6)[:19]]);wants.extend(['{-1,0,0,0,0,0,0}']*2)
    tests=samples('i',vals)+rows('i',vals,wants,'struct {int rc;uint32_t src,dst;unsigned total,proto;size_t header;int frag;} want;')+'''
int main(void){for(size_t i=0;i<sizeof V/sizeof V[0];i++){IPv4 ip={0};int rc=parse_ipv4(V[i].p,V[i].n,&ip);int ok=rc==V[i].want.rc;
 if(!rc)ok=ok&&ip.src==V[i].want.src&&ip.dst==V[i].want.dst&&ip.total==V[i].want.total&&ip.protocol==V[i].want.proto&&ip.header==V[i].want.header&&ip.fragmented==V[i].want.frag;
 CHECK("IPv4 declared boundaries",ok,"datagram %zu",i);}return 0;}'''
    teach('04-ipv4','IPv4 length claims','Separate captured bytes, the variable IP header and the declared datagram length before reading transport data.',
          '''IPv4 carries its own length claims inside a capture. The first byte contains a version nibble and IHL nibble. Require version 4; multiply IHL by four to obtain a header length of at least twenty bytes. Options increase that length, so transport data does not always start at byte twenty.

The total length at bytes 2..3 includes the header and payload. It must be at least the header length and no larger than the supplied byte span. Extra bytes after total length may be Ethernet padding and are not transport data. Protocol is byte 9; source and destination addresses begin at 12 and 16.

The flags/fragment offset field at bytes 6..7 matters before interpreting transport headers. A nonzero offset or More Fragments bit means this datagram participates in fragmentation. The Don't Fragment bit alone does not. This function reports fragmentation but does not reassemble fragments, validate checksum/options, or implement IPv6. Equivalent Wireshark filters: `ip` and `ip.flags.mf == 1 || ip.frag_offset > 0`. The source address remains a packet claim.''',
          'Keep IPv4 as supplied. Implement `int parse_ipv4(const uint8_t *p,size_t n,IPv4 *out)` returning 0 or -1, filling canonical numeric src/dst, total, protocol, header byte length, and fragmented as 0/1. Apply the version/IHL/total-length checks above; failure leaves out unchanged. Checksums and option syntax are outside this subset.',
          BE+IP_TYPE+'\n'+IP_FUNC,IP_TYPE+'\nint parse_ipv4(const uint8_t *p,size_t n,IPv4 *out){(void)p;(void)n;(void)out;return -1;}',tests,
          ('Validate IHL before using it as an offset; total length must be inside the supplied span.', 'The fragment mask is 0x3fff: offset bits plus More Fragments, excluding Dont Fragment.'))

    vals=[];wants=[]
    for payload,s,d,seq,ack,fl,opt in [(b'',40000,80,1000,0,2,b''),(b'hello',80,40000,1001,999,0x18,b''),(b'ab',65535,443,0xffffffff,0x80000000,0x12,b'\1'*12),(b'',1,2,0,0,1,b'\1'*40)]:
        b=tcp(payload,s,d,seq,ack,fl,opt);vals.append(b);wants.append('{0,%d,%d,%uu,%uu,%d,%d,%d}'%(s,d,seq,ack,fl,20+len(opt),len(payload)))
    for off,val in [(12,0x40),(12,0xf0)]:
        b=bytearray(tcp());b[off]=val;vals.append(bytes(b));wants.append('{-1,0,0,0,0,0,0,0}')
    vals.extend([b'',tcp()[:19]]);wants.extend(['{-1,0,0,0,0,0,0,0}']*2)
    tests=samples('t',vals)+rows('t',vals,wants,'struct {int rc;unsigned src,dst;uint32_t seq,ack;unsigned flags;size_t header,payload;} want;')+'''
int main(void){for(size_t i=0;i<sizeof V/sizeof V[0];i++){TCP t={0};int rc=parse_tcp(V[i].p,V[i].n,&t);int ok=rc==V[i].want.rc;if(!rc)ok=ok&&t.src==V[i].want.src&&t.dst==V[i].want.dst&&t.seq==V[i].want.seq&&t.ack==V[i].want.ack&&t.flags==V[i].want.flags&&t.header==V[i].want.header&&t.payload==V[i].want.payload;CHECK("TCP header fields",ok,"segment %zu",i);}return 0;}'''
    teach('05-tcp','Read TCP segments','Decode ports, sequence numbers and flags while respecting the TCP data offset.',
          '''TCP presents a byte stream, but a capture contains individual segments. The first four bytes are source and destination ports, followed by 32-bit sequence and acknowledgement numbers. The upper nibble of byte 12 gives the header length in four-byte words. Byte 13 contains the ordinary FIN/SYN/RST/PSH/ACK/URG/ECE/CWR flags.

Require a complete twenty-byte base header, then validate the data offset before calculating payload length. Options may extend the header up to sixty bytes. The caller has already removed Ethernet/IP headers and restricted the span to the IP payload. Ethernet padding is therefore not counted as TCP data.

A SYN with ACK clear often starts a connection, but a single segment does not establish a complete handshake or an attack. Sequence arithmetic wraps at 2^32 and real reassembly must handle retransmissions, overlaps, loss and bidirectional streams. This lesson only decodes fields and lengths; it does not validate checksums or TCP option syntax. Equivalent filters: `tcp` and `tcp.flags.syn == 1 && tcp.flags.ack == 0`.''',
          'Keep TCP as supplied. Implement `int parse_tcp(const uint8_t *p,size_t n,TCP *out)` returning 0 or -1 for NULL/truncated/invalid offset. Fill canonical src/dst ports, seq/ack, the raw byte-13 flags, header byte length and payload=n-header. Failure leaves out unchanged. The supplied span is a complete IP transport payload.',
          BE+TCP_TYPE+'\n'+TCP_FUNC,TCP_TYPE+'\nint parse_tcp(const uint8_t *p,size_t n,TCP *out){(void)p;(void)n;(void)out;return -1;}',tests,
          ('The TCP offset is `(p[12] >> 4) * 4`, not the whole byte.', 'Check `header >= 20 && header <= n` before subtracting.'),difficulty='easy')

    vals=[];wants=[]
    for payload,s,d in [(b'',40001,53),(b'abc',53,50000),(bytes(range(32)),123,65535)]:
        vals.append(udp(payload,s,d));wants.append('{0,%d,%d,%d}'%(s,d,len(payload)))
    vals.append(udp(b'ab')+b'tail');wants.append('{0,40001,53,2}')
    for size in [0,7,65535]:
        b=bytearray(udp());b[4:6]=struct.pack('!H',size);vals.append(bytes(b));wants.append('{-1,0,0,0}')
    vals.extend([b'',udp()[:7]]);wants.extend(['{-1,0,0,0}']*2)
    tests=samples('u',vals)+rows('u',vals,wants,'struct {int rc;unsigned src,dst;size_t payload;} want;')+'''
int main(void){for(size_t i=0;i<sizeof V/sizeof V[0];i++){UDP u={0};int rc=parse_udp(V[i].p,V[i].n,&u);CHECK("UDP declared length",rc==V[i].want.rc && (rc || (u.src==V[i].want.src&&u.dst==V[i].want.dst&&u.payload==V[i].want.payload)),"datagram %zu",i);}return 0;}'''
    teach('06-udp','UDP datagram bounds','Use the UDP length field to separate its payload from trailing captured bytes.',
          '''UDP has an eight-byte header: source port, destination port, length and checksum, each two bytes in network order. The length includes the header itself, so a value below eight is invalid. A length beyond the supplied transport span indicates a truncated or inconsistent datagram.

Do not assume every captured byte belongs to UDP. After validating `8 <= length <= n`, report `length - 8` as the payload. Trailing bytes are ignored in this bounded teaching interface. The caller supplies the transport payload, not an Ethernet frame, and has already handled IP fragmentation.

UDP has no TCP-style sequence space, acknowledgement or built-in stream ordering. A DNS-looking destination port is only a clue about an application protocol; application bytes must still be parsed. This lesson does not validate checksums: doing so requires the IP pseudo-header and differs between IPv4 and IPv6. Equivalent Wireshark filters are `udp` and `udp.port == 53`. The next lesson reads the DNS message separately so each layer has a clear contract.''',
          'Keep UDP as supplied. Implement `int parse_udp(const uint8_t *p,size_t n,UDP *out)` returning 0 or -1 on NULL, truncation or an invalid declared length. Fill src/dst and payload=declared_length-8; ignore trailing bytes. Do not change out on failure.',
          BE+UDP_TYPE+'\n'+UDP_FUNC,UDP_TYPE+'\nint parse_udp(const uint8_t *p,size_t n,UDP *out){(void)p;(void)n;(void)out;return -1;}',tests,
          ('Read the length at p+4 only after requiring eight bytes.', 'Payload length comes from the declared UDP length, not n.'),difficulty='easy')

    dns_type='typedef struct {uint16_t id,type,qclass;char name[254];} Question;'
    vals=[];wants=[]
    for name,ident,typ,cl in [('notes.example.test',0x1234,1,1),('EXAMPLE.test',99,28,1),('',0,255,3),('a.'+'b'*63+'.test',65535,15,1),('dash-name.v2.test',3,1,1)]:
        vals.append(dns(name,ident,typ,cl));wants.append('{0,%d,%d,%d,%s}'%(ident,typ,cl,cstr(name or '.')))
    vals.extend([b'',dns('example.test')[:-1],struct.pack('!6H',1,0,1,0,0,0)+b'\xc0\x0c\0\1\0\1',struct.pack('!6H',1,0,0,0,0,0)])
    wants.extend(['{-1,0,0,0,""}']*4)
    vals.append(struct.pack('!6H',1,0,1,0,0,0)+b'\x40'+b'a'*64+b'\0\0\1\0\1');wants.append('{-1,0,0,0,""}')
    vals.append(dns('.'.join(['a'*63]*4)));wants.append('{-1,0,0,0,""}')
    tests=samples('d',vals)+rows('d',vals,wants,'struct {int rc;unsigned id,type,qclass;const char *name;} want;')+'''
int main(void){for(size_t i=0;i<sizeof V/sizeof V[0];i++){Question q={0};int rc=dns_question(V[i].p,V[i].n,&q);CHECK("bounded DNS question",rc==V[i].want.rc && (rc || (q.id==V[i].want.id&&q.type==V[i].want.type&&q.qclass==V[i].want.qclass&&!strcmp(q.name,V[i].want.name))),"message %zu",i);}return 0;}'''
    teach('07-dns','Decode one DNS question','Walk length-prefixed DNS labels without following unsupported compression pointers.',
          '''A DNS message has a twelve-byte header. The transaction ID is the first two bytes and QDCOUNT is at bytes 4..5. This subset requires exactly one question. Starting at offset twelve, a name is a sequence of one-byte label lengths followed by label bytes, ending in a zero length. The two 16-bit fields after the name are QTYPE and QCLASS.

Validate every label length (at most 63), every input slice and the output capacity. Join labels with dots while preserving their case. For a root name with no labels, return a single dot. Here the output text limit is 253 characters excluding the terminator; label characters are ASCII letters, digits or hyphens. All other label bytes are rejected by this intentionally small display parser.

Real DNS also uses compression pointers, answers and additional records. A pointer has high bits 11 and must not be mistaken for an ordinary length. Reject it in this exercise; a full decoder would need offset bounds and loop detection. Query and response flags are not interpreted, and trailing answer bytes are ignored. Equivalent filters: `dns` and `dns.qry.name == "notes.example.test"`. The names are fictional .test names, and this code never makes a DNS request.''',
          'Keep Question as supplied. Implement `int dns_question(const uint8_t *p,size_t n,Question *out)` returning 0 or -1. Require n>=12 and QDCOUNT=1; decode an uncompressed first question with supported ASCII label characters, <=63 bytes per label and <=253 display characters. Root displays as `.`. Decode id, type, qclass. Require complete type/class, allow trailing data and leave out unchanged on failure.',
          BE+dns_type+'''
int dns_question(const uint8_t *p,size_t n,Question *out){if(!p||!out||n<12||be16(p+4)!=1)return -1;Question q={0};q.id=be16(p);size_t off=12,k=0;
 while(1){if(off>=n)return -1;size_t len=p[off++];if(!len)break;if(len>63||len>n-off)return -1;if(k){if(k>=253)return -1;q.name[k++]='.';}if(len>253-k)return -1;
  for(size_t i=0;i<len;i++){unsigned c=p[off+i];if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'))return -1;q.name[k++]=(char)c;}off+=len;
 }if(!k)q.name[k++]='.';q.name[k]=0;if(n-off<4)return -1;q.type=be16(p+off);q.qclass=be16(p+off+2);*out=q;return 0;}''',
          dns_type+'\nint dns_question(const uint8_t *p,size_t n,Question *out){(void)p;(void)n;(void)out;return -1;}',tests,
          ('Treat bytes >=64 as unsupported length/pointer values before copying.', 'Reserve room for dots and the final zero independently from the input bounds.'))

    segments='typedef struct {uint32_t seq;const uint8_t *data;size_t n;} Segment;'
    request=b'GET /notes HTTP/1.1\r\nHost: example.test\r\n\r\n'
    tests=arr('request',request)+r'''
int main(void){const size_t cuts[]={1,4,12,20};for(size_t i=0;i<4;i++){size_t k=cuts[i];Segment s[]={{1000+(uint32_t)k,request+k,sizeof request-k},{1000,request,k},{1000,request,k}};char out[32];int rc=http_path(s,3,1000,out,sizeof out);CHECK("out-of-order and duplicate",rc==6&&!strcmp(out,"/notes"),"cut %zu",k);}
 const uint8_t other[]="GET /a/b?q=7 HTTP/1.1\r\n\r\n";Segment s={9,other,sizeof other-1};char out[32];CHECK("another path",http_path(&s,1,9,out,sizeof out)==8&&!strcmp(out,"/a/b?q=7"),"");
 Segment gap[]={{1001,request+1,sizeof request-1}};CHECK("missing first byte",http_path(gap,1,1000,out,sizeof out)==-1,"");
 const uint8_t bad[]="X";Segment conflict[]={{1000,request,sizeof request},{1000,bad,1}};CHECK("conflicting overlap",http_path(conflict,2,1000,out,sizeof out)==-1,"");
 Segment whole={1000,request,sizeof request};CHECK("small output",http_path(&whole,1,1000,out,6)==-1,"");
 Segment partial={1000,request,20};CHECK("incomplete headers",http_path(&partial,1,1000,out,sizeof out)==-1,"");
 const uint8_t wrong[]="POST / HTTP/1.1\r\n\r\n";Segment post={1,wrong,sizeof wrong-1};CHECK("unsupported method",http_path(&post,1,1,out,sizeof out)==-1,"");
 CHECK("empty stream",http_path(NULL,0,1000,out,sizeof out)==-1,"");return 0;}'''
    teach('08-http-stream','Reassemble a tiny HTTP stream','Reorder a bounded one-way TCP byte sequence and extract a GET path only after complete headers arrive.',
          '''Packet boundaries are not application-message boundaries. A request line can be split between segments, and the capture may show segments out of order or more than once. This exercise receives already-decoded segments from one direction of one fictional connection and a known initial sequence number.

Use a 1024-byte temporary buffer plus a presence bitmap. Convert each sequence number to an offset from base, rejecting values before base or ranges outside the buffer. Identical retransmitted bytes are harmless; conflicting overlaps are rejected rather than silently choosing one interpretation. Require every byte from zero through the greatest observed end to be present. Sequence-number wrapping is deliberately unsupported here.

After reassembly, accept a small ASCII HTTP/1.1 subset: `GET `, a nonempty path without spaces, ` HTTP/1.1` and CRLF, followed by complete headers ending CRLF CRLF. Return the path only. This is not a general HTTP parser: bodies, pipelining, chunking and real TCP reassembly remain outside its contract. Equivalent Wireshark view: filter `tcp.port == 80`, then Follow > TCP Stream. A packet filter alone cannot guarantee a complete request.''',
          'Keep Segment as supplied. Implement `int http_path(const Segment *s,size_t count,uint32_t base,char *out,size_t cap)`. Reassemble <=1024 bytes using the stated overlap/gap rules; reject NULL data with positive length, out-of-range segments or invalid/incomplete GET headers. Segments are one direction, with no sequence wrap. Ignore zero-length segments. Return path length and terminate on success; return -1 and leave out unchanged on error. Require a non-NULL output and space for the terminator. Only printable ASCII and CR/LF are accepted in the stream.',
          segments+r'''
int http_path(const Segment *s,size_t count,uint32_t base,char *out,size_t cap){if(!out||!cap||(count&&!s))return -1;uint8_t buf[1025]={0},seen[1024]={0};size_t end=0;
 for(size_t i=0;i<count;i++){if(!s[i].n)continue;if(!s[i].data||s[i].seq<base)return -1;uint32_t delta=s[i].seq-base;if(delta>1024||s[i].n>1024-delta)return -1;size_t off=delta;
  for(size_t j=0;j<s[i].n;j++){size_t k=off+j;if(seen[k]&&buf[k]!=s[i].data[j])return -1;buf[k]=s[i].data[j];seen[k]=1;}if(off+s[i].n>end)end=off+s[i].n;
 }for(size_t i=0;i<end;i++){if(!seen[i]||((buf[i]<32||buf[i]>126)&&buf[i]!='\r'&&buf[i]!='\n'))return -1;}buf[end]=0;
 char *text=(char*)buf;if(end<18||strncmp(text,"GET ",4)||!strstr(text,"\r\n\r\n"))return -1;char *space=strchr(text+4,' ');if(!space||space==text+4||strncmp(space," HTTP/1.1\r\n",11))return -1;
 size_t n=(size_t)(space-(text+4));for(size_t i=0;i<n;i++)if((unsigned char)text[4+i]<33)return -1;if(n>=cap)return -1;memcpy(out,text+4,n);out[n]=0;return (int)n;}''',
          segments+'\nint http_path(const Segment *s,size_t count,uint32_t base,char *out,size_t cap){(void)s;(void)count;(void)base;(void)out;(void)cap;return -1;}',tests,
          ('Track whether each byte has arrived separately from its value; a zero byte is not a presence marker.', 'Reassemble and validate completely before copying the extracted path into out.'))

    flow='typedef struct {uint32_t src;uint16_t port;uint8_t flags;} Flow;'
    flows=[]
    for src,ports in [(0xc000020a,[80,443]),(0xc6336407,[21,22,23,25,80]),(0xcb007109,[1,2,3,4,5,6])]:
        for port in ports:flows.append((src,port,2))
        flows.extend([(src,ports[0],2)]*3+[(src,8080,0x12),(src,9000,0x10)])
    RNG.shuffle(flows)
    data='static const Flow flows[]={'+','.join('{%uu,%d,%d}'%v for v in flows)+'};\n'
    tests=data+'''
int main(void){uint32_t out[8];size_t n=sizeof flows/sizeof flows[0];
 for(size_t threshold=1;threshold<=7;threshold++){size_t count=scan_sources(flows,n,threshold,out,8);uint32_t want[3];size_t k=0;if(threshold<=2)want[k++]=0xc000020a;if(threshold<=5)want[k++]=0xc6336407;if(threshold<=6)want[k++]=0xcb007109;
 CHECK("distinct SYN ports",count==k && !memcmp(out,want,k*sizeof *out),"threshold %zu returned %zu",threshold,count);}
 out[1]=0xdeadbeef;CHECK("output capacity",scan_sources(flows,n,3,out,1)==1 && out[0]==0xc6336407 && out[1]==0xdeadbeef,"");
 CHECK("empty or disabled",scan_sources(NULL,0,3,out,8)==0 && scan_sources(flows,n,0,out,8)==0 && scan_sources(flows,n,1,out,0)==0,"");return 0;}'''
    teach('09-anomaly-summary','Summarize a possible scan','Count distinct destination ports per source while excluding SYN-ACKs and repeated probes.',
          '''A high count of distinct connection attempts can help prioritize traffic review. Counting packets alone is misleading because a lost reply can cause the same SYN to be retransmitted many times. This toy rule counts distinct destination ports per source among flows with SYN set and ACK clear.

For each source, deduplicate the qualifying ports, compare the count with a supplied threshold, then return matching source addresses in numeric order. Other flag bits do not affect the SYN/ACK rule. The input combines all destinations by source; this is a stated simplification, not a full port-scan detector. Threshold zero disables the rule.

This is an observation over the supplied offline summaries. Monitoring agents, authorized scanners, shared addresses and incomplete captures can all produce surprising patterns. Do not label an address malicious from this one count. Equivalent Wireshark filter: `tcp.flags.syn == 1 && tcp.flags.ack == 0`. Statistics > Conversations can help inspect the selected traffic, but a distinct-port count still needs grouping and deduplication. Documentation IPs ensure these examples do not accuse real systems.''',
          'Keep Flow as supplied. Implement `size_t scan_sources(const Flow *f,size_t n,size_t threshold,uint32_t *out,size_t max)`. n<=128, valid spans. Count distinct ports per src only when `(flags & 0x12) == 0x02`. Store qualifying src values once, sorted numerically ascending, at most max; return count stored. Return 0 for threshold=0, n=0 or max=0. No network operations.',
          flow+'''
size_t scan_sources(const Flow *f,size_t n,size_t threshold,uint32_t *out,size_t max){if(!threshold||!max||!n)return 0;uint32_t candidates[128];size_t count=0;
 for(size_t i=0;i<n;i++){int prior=0;for(size_t j=0;j<i;j++)if(f[j].src==f[i].src)prior=1;if(prior)continue;size_t ports=0;
  for(size_t j=0;j<n;j++){if(f[j].src!=f[i].src || (f[j].flags&0x12)!=2)continue;int duplicate=0;for(size_t k=0;k<j;k++)if(f[k].src==f[j].src && f[k].port==f[j].port && (f[k].flags&0x12)==2)duplicate=1;if(!duplicate)ports++;}
  if(ports>=threshold)candidates[count++]=f[i].src;
 }for(size_t i=1;i<count;i++){uint32_t v=candidates[i];size_t j=i;while(j&&candidates[j-1]>v){candidates[j]=candidates[j-1];j--;}candidates[j]=v;}
 size_t stored=count<max?count:max;for(size_t i=0;i<stored;i++)out[i]=candidates[i];return stored;}''',
          flow+'\nsize_t scan_sources(const Flow *f,size_t n,size_t threshold,uint32_t *out,size_t max){(void)f;(void)n;(void)threshold;(void)out;(void)max;return 0;}',tests,
          ('Consider each source once, then count qualifying ports only on their first qualifying occurrence.', 'Sort candidate addresses before applying max so truncation returns the lowest addresses.'))

    # A review reuses the exact field parsers as supplied support code.
    vals=[eth(arp(),0x0806),eth(ipv4(tcp(),6)),eth(ipv4(udp(dns('example.test')),17)),eth(ipv4(udp(b'x',1,2),17)),eth(ipv4(b'ab',1)),eth(ipv4(b'xx',6,frag=0x2000)),eth(b'x',0x86dd),b'',eth(ipv4(tcp()[:5],6)),eth(ipv4(udp()[:5],17))]
    wants=list(map(cstr,['arp','ipv4/tcp','ipv4/udp/dns-port','ipv4/udp','ipv4/other','ipv4/fragment','other','invalid','invalid','invalid']))
    vals.append(eth(ipv4(udp(b'x'),17),vlan=9));wants.append(cstr('ipv4/udp/dns-port'))
    vals.append(eth(ipv4(tcp(),6)[:-1]));wants.append(cstr('invalid'))
    tests=samples('r',vals)+rows('r',vals,wants,'const char *want;')+'''
int main(void){for(size_t i=0;i<sizeof V/sizeof V[0];i++){const char *got=classify(V[i].p,V[i].n);CHECK("layered classification",got && !strcmp(got,V[i].want),"frame %zu expected %s got %s",i,V[i].want,got?got:"NULL");}return 0;}'''
    support=ETH_CODE+'\n'+IP_TYPE+'\n'+IP_FUNC+'\n'+TCP_TYPE+'\n'+TCP_FUNC+'\n'+UDP_TYPE+'\n'+UDP_FUNC+'\n'
    teach('10-review-layered-analysis','Review: layered analysis','Combine established layer bounds before classifying a frame and distinguish a port hint from a parsed DNS message.',
          '''This review combines [Ethernet](/learn/sec-packets/ethernet), [IPv4](/learn/sec-packets/ipv4), [TCP](/learn/sec-packets/tcp) and [UDP](/learn/sec-packets/udp). The supplied helpers are the same bounded field parsers; use their outputs to decide exactly which bytes belong to the next layer.

Start with Ethernet and return other for unsupported types. For IPv4, constrain the transport span to total length minus the actual header length, ignoring padding. Return a fragment classification before trying to decode a transport header. A later fragment may contain no TCP/UDP header at all.

Use the transport parser before labeling TCP or UDP. Port 53 is only a DNS-port hint, so the label deliberately says dns-port rather than asserting a valid DNS message. The earlier DNS parser would be needed to inspect the application bytes. No new protocol is introduced. Equivalent filter: `arp || ip`; compare each classification with the protocol tree in Wireshark. The goal is a reproducible description with explicit limits, not a judgment about whether traffic is safe.''',
          'Implement `const char *classify(const uint8_t *frame,size_t n)` using supplied helpers. Return invalid if Ethernet, IPv4 or a selected TCP/UDP header fails. EtherType 0x0806 returns arp (EtherType observation only); other non-IPv4 types return other. IPv4 fragments return ipv4/fragment. Complete TCP returns ipv4/tcp; complete UDP returns ipv4/udp/dns-port if either port=53, else ipv4/udp. Other IP protocols return ipv4/other. Respect IPv4 total length and IHL.',
          support+'''
const char *classify(const uint8_t *frame,size_t n){Ethernet e;if(parse_ethernet(frame,n,&e))return "invalid";if(e.type==0x0806)return "arp";if(e.type!=0x0800)return "other";
 IPv4 ip;if(parse_ipv4(frame+e.payload,n-e.payload,&ip))return "invalid";if(ip.fragmented)return "ipv4/fragment";const uint8_t *p=frame+e.payload+ip.header;size_t len=ip.total-ip.header;
 if(ip.protocol==6){TCP t;return parse_tcp(p,len,&t)?"invalid":"ipv4/tcp";}
 if(ip.protocol==17){UDP u;if(parse_udp(p,len,&u))return "invalid";return u.src==53||u.dst==53?"ipv4/udp/dns-port":"ipv4/udp";}return "ipv4/other";}''',
          support+'const char *classify(const uint8_t *frame,size_t n){(void)frame;(void)n;return "invalid";}',tests,
          ('Use Ethernet.payload, then IPv4.header; neither layer always has its minimum length.', 'Handle fragments before transport parsing and pass only total-header bytes to the transport helper.'),review=True)
    print('sec-packets lessons and synthetic evidence generated')


if __name__=='__main__':
    main()
