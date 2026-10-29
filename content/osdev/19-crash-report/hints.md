`exception_name`: a `static const char *const names[22] = {"Divide Error", "Debug", ...};` and `if (vector < 22) return names[vector]; return vector < 32 ? "Reserved" : "External Interrupt";`. Vector 15 sits inside the table as `"Reserved"`.
===
`exception_has_error_code` is a `switch` with the ten vectors as `case` labels returning 1. Or test a bitmask: `(0x60227D00ul >> vector) & 1` for vectors below 32 (check the arithmetic yourself!).
===
For `describe_page_fault`, keep `pos` as you go: `size_t pos = 0; pos = put_str(out, cap, pos, (err & 16) ? "fetch from" : (err & 2) ? "write to" : "read from"); pos = put_str(out, cap, pos, " a "); ...`. Call `put_str(out, cap, 0, "")` first if `cap` could be zero-length output, so the buffer starts as an empty string.
===
`describe_selector_error`: return `"no selector"` via `put_str` for `err == 0`. Otherwise `static const char *const tables[4] = {"GDT", "IDT", "LDT", "IDT"}; tables[(err >> 1) & 3]`, then `" index "`, `put_dec(..., err >> 3)`, and `" (external)"` when `err & 1`.
===
`format_crash` builds the first part, then a chain of `if`s for 14 and 10..13. Reuse the two describe functions by writing them into a local `char tmp[96]` and appending with `put_str`. For `crash_init`, the handler has the signature `void h(uint64_t vector, uint64_t error_code, uint64_t rip)`; read CR2 with `__asm__ volatile("mov %%cr2, %0" : "=r"(cr2))`.
