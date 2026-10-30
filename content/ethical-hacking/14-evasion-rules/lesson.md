---
title: Obfuscation and the signature gap
summary: Write a literal detection rule for a made-up script language, watch disguised variants slip past, then rebuild the rule around normalisation and decoding.
minutes: 45
difficulty: hard
tags: evasion, detection, signatures, malware-analysis
id: ethical-hacking/evasion-rules
mode: harness
---
> **Simulated target.** Everything in this lesson is made up and runs inside the sandbox: the 'samples' are strings in an invented toy script language (bmscript) that does nothing. The behaviour in question is a made-up NET BEACON statement. In scope: the detection rules you write. No real malware, product or scanner is involved, and nothing here targets one. Never use a technique from this course on a system you do not own or do not have written permission to test. Start with [Law, ethics and permission](/learn/ethical-hacking/law-and-ethics) if you have not yet.

Defenders often start with a **signature**: a pattern that identifies something known to be bad. Attackers know that, so the same behaviour is written in a way the pattern no longer matches. This is **evasion**. It is a cat-and-mouse game that defenders only win by moving from *spelling* to *meaning*. You will play both sides in a toy setting.

The made-up language *bmscript* has statements, one per line:

```text
SET host=update.example.test
NET connect update.example.test
NET beacon update.example.test      <- the behaviour we want to catch
ECHO "hello"
# a comment
```

## Round 1: the literal rule

`detect_v1` is the obvious first rule: the sample contains the text `NET BEACON`. It works on the sample you analysed. Then three variants appear, all with the same behaviour:

- `net  Beacon` and `NET <tab>beacon`: the case and spacing differ. (The rule is case-sensitive and spacing-sensitive.)
- `NET "bea"+"con"`: the word is built from pieces. (The text never appears.)
- `PACK xor=5a 1f3f...`: the whole script is hidden as hex bytes XORed with a one-byte key, a toy version of **packing**. The meaning is only visible after decoding.

And there are false alarms: a comment saying `# NET BEACON is disabled` or `ECHO "NET BEACON"` is text, not behaviour. The v1 rule flags both.

## Round 2: normalise, decode, then look at structure

`detect_v2` handles all of that by analysing what the statement *means*:

1. Ignore comment lines (first non-blank character `#`).
2. A line `PACK xor=KK HEX...` (keyword in any case; `KK` two hex digits; the hex bytes follow after a space) decodes to more script, which is analysed as lines too. Packs may nest, up to 3 levels deep. Stop decoding at a bad hex pair.
3. For other lines, delete every `"` and every `+`, lower-case, squeeze runs of spaces and tabs to one space and trim.
4. The line is a hit if it is exactly `net beacon`, or starts with `net beacon ` (a space after).

This is still a toy rule. A determined author has endless other ways (encodings you do not decode, logic computed at run time). That is the lesson: **a rule only covers what it models**, so defenders layer rules with behaviour monitoring and log what they cannot decode. Keep that in mind when you write the report for [a finding like this](/learn/ethical-hacking/finding-report).
