static size_t piece(unsigned char c,char*p){static const char hx[]="0123456789abcdef";
 if(c=='\n'){memcpy(p,"\\n",2);return 2;}if(c=='\r'){memcpy(p,"\\r",2);return 2;}if(c=='\\'){memcpy(p,"\\\\",2);return 2;}
 if(c<0x20||c==0x7f){p[0]='\\';p[1]='x';p[2]=hx[c>>4];p[3]=hx[c&15];return 4;}p[0]=(char)c;return 1;}
int main(void){
 const char*in[]={"plain","a\nb","x\r\ny\\z","\x01\x7f","","tab\there","user=\"bob\"\nINFO login ok","caf\xc3\xa9\x80","\\n literal","\x1f\x20\x7e"};
 for(size_t k=0;k<sizeof in/sizeof in[0];k++){
  char full[400];size_t total=0;for(const unsigned char*p=(const unsigned char*)in[k];*p;p++)total+=piece(*p,full+total);full[total]=0;
  CHECK("return value covers the whole text",log_escape(NULL,0,in[k])==total,"input %zu",k);
  for(size_t cap=0;cap<=total+2;cap++){
   uint8_t raw[8+400+8];memset(raw,0xA5,sizeof raw);
   size_t rc=log_escape((char*)raw+8,cap,in[k]);
   int guards=1;for(size_t i=0;i<8;i++)if(raw[i]!=0xA5||raw[8+cap+i]!=0xA5)guards=0;
   char want[400];size_t w=0;int stop=0;for(const unsigned char*p=(const unsigned char*)in[k];*p;p++){char pc[4];size_t l=piece(*p,pc);
    if(!stop&&cap&&w+l<cap){memcpy(want+w,pc,l);w+=l;}else stop=1;}
   int body=cap==0?raw[8]==0xA5:(raw[8+w]==0&&!memcmp(raw+8,want,w));
   CHECK("capacity sweep",rc==total&&guards&&body,"input %zu cap %zu: returned %zu (want %zu)",k,cap,rc,total);}}
 char o[8]="zzzzzzz";
 CHECK("NULL input",log_escape(o,8,NULL)==0&&o[0]==0,"");
 CHECK("NULL output",log_escape(NULL,10,"a\nb")==4,"");

    /* Extra cases: exact outputs, truncation rules and a round trip through a decoder written in the test. */
    {
        char o[256];
        size_t rc;

        rc = log_escape(o, sizeof o, "bob\nINFO login ok user=admin");
        CHECK("a forged log line becomes a single line", rc == 28 + 1 && strcmp(o, "bob\\nINFO login ok user=admin") == 0 && strchr(o, '\n') == NULL, "got \"%s\" (returned %zu)", o, rc);
        rc = log_escape(o, sizeof o, "a\tb");
        CHECK("tab is a control byte: \\x09", strcmp(o, "a\\x09b") == 0 && rc == 6, "got \"%s\" (returned %zu)", o, rc);
        rc = log_escape(o, sizeof o, "x\\ny");
        CHECK("a literal backslash-n in the input is doubled so it stays distinguishable", strcmp(o, "x\\\\ny") == 0 && rc == 5, "got \"%s\" (returned %zu)", o, rc);
        rc = log_escape(o, sizeof o, "\x1b[2J\x7f\x1f ~");
        CHECK("escape sequences, DEL and the last control byte are hex escaped; space and tilde are not", strcmp(o, "\\x1b[2J\\x7f\\x1f ~") == 0, "got \"%s\" (returned %zu)", o, rc);
        rc = log_escape(o, sizeof o, "caf\xc3\xa9 \x80\xff");
        CHECK("bytes 0x80 and above are copied unchanged", strcmp(o, "caf\xc3\xa9 \x80\xff") == 0 && rc == 8, "returned %zu", rc);
        rc = log_escape(o, sizeof o, "\r\n");
        CHECK("carriage return and newline", strcmp(o, "\\r\\n") == 0 && rc == 4, "got \"%s\"", o);

        memset(o, 'Z', sizeof o);
        rc = log_escape(o, 6, "ab\x01");
        CHECK("a piece must fit together with the terminator or it is not written at all", rc == 6 && strcmp(o, "ab") == 0, "cap 6: \\x01 needs 4 characters plus the terminator, expected \"ab\" and 6, got \"%s\" and %zu", o, rc);
        rc = log_escape(o, 4, "ab\x01" "c");
        CHECK("after the first piece that does not fit, nothing more is written", rc == 7 && strcmp(o, "ab") == 0, "cap 4: \\x01 needs 4 characters, the later 'c' must not be written; got \"%s\" and %zu", o, rc);
        memset(o, 'Z', sizeof o);
        rc = log_escape(o, 1, "abc");
        CHECK("cap 1 gives an empty string and the full length", rc == 3 && o[0] == 0 && o[1] == 'Z', "got %zu and o[0] = %d", rc, o[0]);

        /* round trip: decode what log_escape produced, for pseudo-random byte strings (never 0) */
        uint32_t x = 7;
        int ok = 1;
        size_t bad_len = 0;
        for (int t = 0; t < 200 && ok; t++) {
            char in[80], enc[400], dec[100];
            size_t n = (size_t)(t % 60);
            for (size_t i = 0; i < n; i++) {
                x = x * 1664525u + 1013904223u;
                unsigned v = (x >> 24) % 255 + 1;
                if (t % 3 == 0) v = "\n\r\\\t\x1b\x7f a"[(x >> 20) % 8];
                in[i] = (char)v;
            }
            in[n] = 0;
            size_t need = log_escape(enc, sizeof enc, in);
            size_t d = 0;
            for (size_t i = 0; i < need && ok; i++) {
                unsigned char c = (unsigned char)enc[i];
                if (c < 0x20 || c == 0x7f || c == '\n') { ok = 0; break; }
                if (c != '\\') { dec[d++] = (char)c; continue; }
                char nx = enc[++i];
                if (nx == 'n') dec[d++] = '\n';
                else if (nx == 'r') dec[d++] = '\r';
                else if (nx == '\\') dec[d++] = '\\';
                else if (nx == 'x') { dec[d++] = (char)(((enc[i + 1] <= '9' ? enc[i + 1] - '0' : enc[i + 1] - 'a' + 10) << 4) | (enc[i + 2] <= '9' ? enc[i + 2] - '0' : enc[i + 2] - 'a' + 10)); i += 2; }
                else ok = 0;
            }
            dec[d] = 0;
            if (!ok || d != n || memcmp(dec, in, n) != 0) { ok = 0; bad_len = n; }
        }
        CHECK("escaping is reversible and never emits a raw control byte", ok, "an input of %zu bytes did not decode back to itself, or the output contained a raw control byte", bad_len);
    }

 return 0;}
