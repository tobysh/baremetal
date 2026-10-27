ENTRY(_start)
SECTIONS {
  . = 1M;
  .boot : { *(.multiboot) *(.boot) }
  .text : { *(.text*) }
  .rodata : { *(.rodata*) }
  .data : { *(.data*) }
  .bss : { *(.bss*) *(COMMON) }
  _kernel_end = .;
}
