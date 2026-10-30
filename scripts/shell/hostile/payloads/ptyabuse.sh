# Hostile bytes on the PTY: very long lines, binary garbage, control and escape sequences, bad UTF-8, terminal
# queries that make the terminal answer INTO the shell's stdin, and tty ioctl abuse. Expect: the session stays
# usable (the driver sends a sentinel afterwards) and the broker/frames layer is unaffected.
long=$(printf 'A%.0s' {1..60000})
printf '%s\n' "$long"                       # 60 KB line, no spaces
printf '%s' "$long$long$long"               # 180 KB with no newline at all
head -c 300000 /dev/urandom                 # binary garbage, includes bytes that look like frame headers
head -c 100000 /dev/zero                    # NULs
printf '\033[2J\033[3J\033[H\033c'          # clear, scrollback clear, full reset
printf '\033]0;%s\007' "$(printf 'T%.0s' {1..20000})"   # huge window title (OSC 0)
printf '\033]52;c;%s\007' "$(printf 'QUJD%.0s' {1..20000})"   # OSC 52 clipboard write
printf '\033]52;c;?\007'                    # OSC 52 clipboard READ request
printf '\033[6n\033[c\033[>c\033Z\005'      # cursor/device attribute queries and ENQ: replies come back as input
printf '\033[?1049h\033[?1000h\033[?1003h\033[?2004h\033[?25l'   # alt screen, mouse reporting, bracketed paste, hide cursor
printf '\033[999999999C\033[999999999B\033[8;99999;99999t\033[18t\033[14t'   # huge moves, resize/report requests
printf '\033]8;;http://169.254.169.254/\033\\link\033]8;;\033\\\n'   # hyperlink to a metadata address (terminal must not follow)
printf '\xc3\x28\xa0\xa1\xe2\x28\xa1\xf0\x28\x8c\xbc\xff\xfe \xe2\x80\xae reversed \xe2\x80\xad\n'   # invalid UTF-8, bidi overrides
printf '\x9b2J\x9d0;x\x9c\x90\x9c\x1b\x1b\x1b\x1b\n'   # C1 controls, DCS, stray ESCs
printf '\033[?1049l\033[?1000l\033[?1003l\033[?25h\033c'   # put the terminal back
stty -a >/dev/null 2>&1 && bm_pass ptyabuse.stty "stty works"
bm_denied ptyabuse.ioctl-baud bash -c 'stty 0 </dev/null'   # no tty on stdin: must fail, not hang
stty rows 65535 cols 65535 2>/dev/null; stty rows 0 cols 0 2>/dev/null; stty raw -echo isig 2>/dev/null; stty sane 2>/dev/null
bm_pass ptyabuse.survived "shell still running after $(printf '%s' "$long" | wc -c) + garbage bytes"
bm_done
