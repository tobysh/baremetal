Split the work into small helpers you already wrote: one that names the signature, one that computes the checksum. The report itself is then mostly formatting.
===
Build the preview in a local buffer first. Add a space before every byte except the first, stop after eight bytes, and put a single `-` there when `n` is zero.
===
GIF and ELF are recognised by the earlier lesson but are not part of this report, so map any type outside PNG, JPEG, PDF and ZIP to `unknown`. A truncated signature has to give `unknown` too.
===
Format the whole line with one `snprintf(out, cap, ...)`: it already truncates safely, terminates when `cap > 0` and writes nothing when `cap` is 0. Print the computed digest with `%016llx` and a cast, and pick `OK` or `CHANGED` by comparing it with `expected`.
