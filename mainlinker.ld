/*
******************************************************************************
**
** @file        : mainlinker.ld
**
** @author      : buh07
**
**  Abstract    : Linker script for NUCLEO-L476RG Board embedding STM32L476RGTx Device from stm32l4 series
**                      1024KBytes FLASH
**                      96KBytes RAM
**                      32KBytes RAM2
**
**                Set heap size, stack size and stack location according
**                to application requirements for the main application.
**
**                Styled after the STM32CubeIDE generated linker script
**
******************************************************************************
*/

ENTRY(Reset_Handler)

/* MEMORY regions definition for STM32L476RG dual-bank system */

MEMORY
{
  /* === FLASH BANK 1 === */
  /* MAIN_APPLICATION occupies the full 512 KB of Bank 1 */
  MAIN_APP   (rx)  : ORIGIN = 0x08000000, LENGTH = 512K

  /* === FLASH BANK 2 === */
  /* BOOTLOADER occupies the first 32 KB of Bank 2 */
  BOOTLOADER (rx)  : ORIGIN = 0x08080000, LENGTH = 32K

  /* UPDATE_IMG follows BOOTLOADER (32K offset), 240 KB */
  UPDATE_IMG (rx)  : ORIGIN = 0x08088000, LENGTH = 240K

  /* BACKUP_APP follows UPDATE_IMG (240K offset), 240 KB */
  BACKUP_APP (rx)  : ORIGIN = 0x080C8000, LENGTH = 240K

  /*
   * Note:
   * 0x08080000 — FLASH Bank 2 Start
   *   +0x8000   — BOOTLOADER
   *   +0x3C000  — UPDATE_IMG
   *   +0x3C000  — BACKUP_APP
   * => 0x08080000 + 32K + 240K + 240K = 0x08100000 (end of Bank 2, 512 KB total)
   */

  /* === SRAM === */
  RAM        (xrw) : ORIGIN = 0x20000000, LENGTH = 96K     /* Main SRAM */
  RAM2       (xrw) : ORIGIN = 0x10000000, LENGTH = 32K     /* Additional RAM */
}


/* Core System Definitions */
_Min_Heap_Size  = 0x800;      /* Minimum 2KB heap */
_Min_Stack_Size = 0x800;      /* Minimum 2KB stack */
__Stack_Size    = _Min_Stack_Size;

/* Symbol Exports for Runtime */
__ld_flash_start__   = ORIGIN(FLASH);
__ld_flash_end__     = ORIGIN(FLASH) + LENGTH(FLASH);
__ld_ram_start__     = ORIGIN(RAM);
__ld_ram_end__       = ORIGIN(RAM) + LENGTH(RAM);
__ld_ram2_start__    = ORIGIN(RAM2);
__ld_ram2_end__      = ORIGIN(RAM2) + LENGTH(RAM2);
__ld_stack_top__     = ORIGIN(RAM) + LENGTH(RAM);
__ld_heap_base__     = _sheap;
__ld_heap_limit__    = _eheap;

SECTIONS {
  /* ============= FLASH Sections ============= */
  
  /* Interrupt Vector Table (256-byte aligned) */
  .isr_vector : {
    . = ALIGN(256);
    KEEP(*(.isr_vector))      /* Must be kept */
    . = ALIGN(4);
  } > FLASH

  /* Code Section */
  .text : {
    *(.text .text.*)          /* All code sections */
    KEEP(*(.init))            /* Startup code */
    KEEP(*(.fini))            /* Termination code */
    *(.gnu.linkonce.t.*)
    . = ALIGN(4);
    _etext = .;               /* End of text section */
  } > FLASH

  /* Read-only Data */
  .rodata : {
    *(.rodata .rodata.*)
    *(.constdata .constdata.*)
    . = ALIGN(4);
  } > FLASH

  /* ARM Exception Handling */
  .ARM.extab : {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } > FLASH
  
  .ARM.exidx : {
    __exidx_start = .;
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    __exidx_end = .;
  } > FLASH

  /* Pre-initialization Array */
  .preinit_array : {
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array*))
    PROVIDE_HIDDEN(__preinit_array_end = .);
  } > FLASH

  /* Initialization Array */
  .init_array : {
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array*))
    PROVIDE_HIDDEN(__init_array_end = .);
  } > FLASH

  /* Termination Array */
  .fini_array : {
    PROVIDE_HIDDEN(__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array*))
    PROVIDE_HIDDEN(__fini_array_end = .);
  } > FLASH

  /* ============= RAM Sections ============= */
  
  /* Initialized Data (Load address in FLASH, runtime in RAM) */
  .data : {
    . = ALIGN(4);
    _sdata = .;               /* Start of .data in RAM */
    *(.data .data.*)
    . = ALIGN(4);
    _edata = .;               /* End of .data in RAM */
  } > RAM AT > FLASH
  _sidata = LOADADDR(.data);  /* Load address in FLASH */

  /* Uninitialized Data (BSS) */
  .bss (NOLOAD) : {
    . = ALIGN(4);
    _sbss = .;
    *(.bss .bss.*)
    *(COMMON)                 /* Common symbols */
    . = ALIGN(4);
    _ebss = .;
  } > RAM

  /* DMA/Buffer Section in RAM2 */
  .ram2 (NOLOAD) : {
    *(.ram2 .ram2.*)
  } > RAM2

  /* ============= Special Sections ============= */
  
  /* Heap Section */
  .heap (NOLOAD) : {
    . = ALIGN(8);
    _sheap = .;
    . = . + _Min_Heap_Size;
    . = ALIGN(8);
    _eheap = .;
  } > RAM

  /* Stack Section (placed at end of RAM) */
  .stack (NOLOAD) : {
    . = ALIGN(8);
    . = . + __Stack_Size;
    . = ALIGN(8);
    _estack = .;
  } > RAM

  /* Application Flags in Special Flash Sector */
  .app_flags (NOLOAD) : {
    KEEP(*(.app_flags))       /* Must be kept */
  } > B1_FLAGS

  /* ============= Debug Sections ============= */
  
  /* Stabs debugging sections */
  .stab          0 : { *(.stab) }
  .stabstr       0 : { *(.stabstr) }
  .stab.excl     0 : { *(.stab.excl) }
  .stab.exclstr  0 : { *(.stab.exclstr) }
  .stab.index    0 : { *(.stab.index) }
  .stab.indexstr 0 : { *(.stab.indexstr) }
  
  /* DWARF debug sections */
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_sfnames  0 : { *(.debug_sfnames) }
  
  /* Discard Unnecessary Sections */
  /DISCARD/ : {
    *(.comment)
    *(.note.*)
    *(.gnu.*)
  }
}