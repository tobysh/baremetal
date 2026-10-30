static int toy_win(const uint8_t *frame, size_t offset, uint64_t win) {
 uint64_t ret=0; for(size_t j=0;j<8;j++)ret|=(uint64_t)frame[offset+j]<<(8*j);
 return ret==win;
}
int main(void) {
 for(size_t offset=8;offset<=80;offset+=8) {
  uint8_t payload[96],frame[112],before[112];
  uint64_t win=UINT64_C(0x004012000000007f)+offset*257;
  memset(payload,0xcc,sizeof payload);size_t n=make_payload(payload,sizeof payload,offset,win);
  int shape=n==offset+8;for(size_t j=offset+8;j<sizeof payload;j++)if(payload[j]!=0xcc)shape=0;
  CHECK("payload length and trailing guards",shape,"offset=%zu",offset);
  memset(frame,0xa5,sizeof frame);
  if(n<=sizeof payload && n<=sizeof frame)memcpy(frame,payload,n);
  CHECK("modeled return reaches win",toy_win(frame,offset,win),"offset=%zu",offset);
  memset(frame,0xa5,sizeof frame);memcpy(before,frame,sizeof frame);
  CHECK("repair rejects overflow",safe_copy(frame,offset,payload,offset+8)==0 && memcmp(frame,before,sizeof frame)==0,"");
  int ok=safe_copy(frame,offset,payload,offset)==1 && memcmp(frame,payload,offset)==0;
  for(size_t j=offset;j<sizeof frame;j++)if(frame[j]!=0xa5)ok=0;
  CHECK("repair preserves saved return",ok,"");
  memset(payload,0xcc,sizeof payload);memcpy(before,payload,sizeof payload);
  CHECK("small output unchanged",make_payload(payload,offset+7,offset,win)==0 && memcmp(payload,before,sizeof payload)==0,"");
 }
 uint8_t b[16];memset(b,0x5a,sizeof b);
 CHECK("size arithmetic overflow",make_payload(b,sizeof b,SIZE_MAX-3,1)==0 && b[0]==0x5a,"");
 CHECK("null output",make_payload(NULL,32,8,1)==0,"");
 CHECK("zero copy",safe_copy(NULL,0,NULL,0)==1,"");
 CHECK("null pointers",safe_copy(NULL,8,b,1)==0 && safe_copy(b,8,NULL,1)==0,"");
 return 0;
}
