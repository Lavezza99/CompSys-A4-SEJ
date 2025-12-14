/* Linker script for RISC-V test programs */

OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS
{
  . = 0x10000;
  
  .text : {
    *(.text)
    *(.text.*)
  }
  
  . = ALIGN(4);
  
  .data : {
    *(.data)
    *(.data.*)
    *(.rodata)
    *(.rodata.*)
  }
  
  . = ALIGN(4);
  
  .bss : {
    *(.bss)
    *(.bss.*)
  }
  
  . = ALIGN(4);
  _end = .;
}