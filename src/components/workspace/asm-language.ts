import { StreamLanguage, type StringStream } from "@codemirror/language";
import { tags as t } from "@lezer/highlight";

// x86-64 GNU as, Intel syntax (`.intel_syntax noprefix`). Loaded lazily by the
// editor, only for assembly lessons.

const MNEMONICS = new Set(
  (
    "mov movzx movsx movsxd movabs lea xchg push pop pushf popf cdq cqo cwd cwde cdqe cbw bswap " +
    "add adc sub sbb inc dec neg not and or xor test cmp mul imul div idiv shl shr sal sar rol ror rcl rcr shld shrd " +
    "bt bts btr btc bsf bsr popcnt lzcnt tzcnt andn blsi blsr blsmsk " +
    "jmp call ret leave enter loop loope loopne syscall sysret int int3 iret iretq hlt nop pause ud2 cpuid rdtsc rdtscp " +
    "cmpxchg cmpxchg8b cmpxchg16b xadd lock rep repe repz repne repnz " +
    "movs movsb movsw movsd movsq stos stosb stosw stosd stosq lods lodsb lodsw lodsd lodsq scas scasb scasw scasd scasq cmps cmpsb cmpsw cmpsd cmpsq " +
    "cld std clc stc cmc sti cli lahf sahf in out ins outs insb insw insd outsb outsw outsd " +
    "lgdt lidt lldt ltr sgdt sidt invlpg wrmsr rdmsr sysenter sysexit swapgs xgetbv " +
    "mfence lfence sfence prefetcht0 prefetcht1 prefetcht2 prefetchnta " +
    "movd movq movdqa movdqu movaps movups movapd movupd movss movsd movhps movlps movhlps movlhps movmskpd movmskps " +
    "paddb paddw paddd paddq psubb psubw psubd psubq pmullw pmulld pmuludq pand pandn por pxor pcmpeqb pcmpeqw pcmpeqd pcmpgtb pmovmskb " +
    "pshufd pshufb pshufhw pshuflw punpcklbw punpcklwd punpckldq punpcklqdq punpckhbw punpckhwd punpckhdq punpckhqdq " +
    "psllw pslld psllq psrlw psrld psrlq psraw psrad pslldq psrldq palignr pminub pmaxub psadbw " +
    "addss addsd addps addpd subss subsd subps subpd mulss mulsd mulps mulpd divss divsd divps divpd sqrtss sqrtsd " +
    "minss minsd maxss maxsd andps andpd andnps andnpd orps orpd xorps xorpd ucomiss ucomisd comiss comisd " +
    "cvtsi2ss cvtsi2sd cvttss2si cvttsd2si cvtss2sd cvtsd2ss cvtdq2ps cvtps2dq shufps unpcklps unpckhps " +
    "fld fst fstp fild fistp fadd fsub fmul fdiv fxch fchs fabs fsqrt fcom fcomi fucomi fldz fld1 fninit"
  ).split(/\s+/),
);

// jcc / cmovcc / setcc families: j, cmov or set followed by a condition code.
const CONDITION = "(?:a|ae|b|be|c|e|g|ge|l|le|na|nae|nb|nbe|nc|ne|ng|nge|nl|nle|no|np|ns|nz|o|p|pe|po|s|z)";
const COND_OP = new RegExp(`^(?:j${CONDITION}|cmov${CONDITION}|set${CONDITION}|jrcxz|jecxz)$`);

const REGISTERS = new RegExp(
  "^(?:" +
    "[re]?(?:ax|bx|cx|dx|si|di|bp|sp|ip)|[abcd][lh]|(?:sil|dil|bpl|spl)|r(?:8|9|1[0-5])[dwb]?|" +
    "[xyz]mm(?:[0-9]|[12][0-9]|3[01])|mm[0-7]|st(?:\\([0-7]\\))?|[cdefgs]s|" +
    "cr[0-9]+|dr[0-7]|rflags|eflags|flags|k[0-7]" +
  ")$",
  "i",
);

const SIZE_WORDS = new Set(
  "byte word dword qword tbyte oword xmmword ymmword zmmword ptr offset short near far rip".split(" "),
);

const tokenTable = {
  mnemonic: t.keyword,
  register: t.typeName,
  size: t.modifier,
  directive: t.processingInstruction,
  label: t.function(t.variableName),
};

function token(stream: StringStream): string | null {
  if (stream.eatSpace()) return null;

  if (stream.match("//") || stream.eat("#")) {
    stream.skipToEnd();
    return "comment";
  }
  if (stream.match("/*")) {
    while (!stream.eol()) if (stream.next() === "*" && stream.eat("/")) break;
    return "comment";
  }

  const quote = stream.peek();
  if (quote === '"' || quote === "'") {
    stream.next();
    let escaped = false;
    let c: string | void;
    while ((c = stream.next())) {
      if (c === quote && !escaped) break;
      escaped = !escaped && c === "\\";
    }
    return "string";
  }

  // Numbers: 0x1F, 0b101, 1010b, 0FFh, 42, -3 (sign handled as punctuation).
  if (stream.match(/^0[xX][0-9a-fA-F]+/) || stream.match(/^0[bB][01]+\b/) || stream.match(/^[0-9][0-9a-fA-F]*[hH]\b/) || stream.match(/^[01]+[bB]\b/) || stream.match(/^[0-9]+\b/)) {
    return "number";
  }

  // Directives and local labels: .globl, .Lloop (a trailing colon makes it a label).
  if (stream.match(/^\.[A-Za-z_.$][\w.$]*/)) {
    const text = stream.current();
    if (stream.peek() === ":") {
      stream.next();
      return "label";
    }
    return /^\.L/.test(text) ? "variableName" : "directive";
  }

  if (stream.match(/^[A-Za-z_$@][\w.$@]*/)) {
    const word = stream.current();
    if (stream.peek() === ":") {
      stream.next();
      return "label";
    }
    const lower = word.toLowerCase();
    if (SIZE_WORDS.has(lower)) return "size";
    if (REGISTERS.test(word)) return "register";
    if (MNEMONICS.has(lower) || COND_OP.test(lower)) return "mnemonic";
    return "variableName";
  }

  stream.next();
  return null;
}

export const asm = StreamLanguage.define({
  name: "asm",
  token,
  tokenTable,
  languageData: { commentTokens: { line: "#" } },
});
