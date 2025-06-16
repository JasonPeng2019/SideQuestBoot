/*
******************************************************************************
**
** @file        : bootloaderlinker.ld
**
** @author      : buh07
**
**  Abstract    : Linker script for NUCLEO-L476RG Board embedding STM32L476RGTx Device from stm32l4 series
**                      1024KBytes FLASH
**                      96KBytes RAM
**                      32KBytes RAM2
**
**                Set heap size, stack size and stack location according
**                to application requirements for the bootloader.
**
**                Styled after the STM32CubeIDE generated linker script
**
******************************************************************************
*/

ENTRY(Reset_Handler)

/* Memory Regions */
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

/* Heap/Stack Sizes (2KB each by default) */
_Min_Heap_Size  = 0x800;
_Min_Stack_Size = 0x800;

/* Symbol Exports */
__bootloader_start__ = ORIGIN(BOOTLOADER);
__bootloader_end__   = ORIGIN(BOOTLOADER) + LENGTH(BOOTLOADER);
__main_app_start__   = ORIGIN(MAIN_APP);
__main_app_end__     = ORIGIN(MAIN_APP) + LENGTH(MAIN_APP);
__ram2_start__       = ORIGIN(RAM2);
__ram2_end__         = ORIGIN(RAM2) + LENGTH(RAM2);
_estack              = ORIGIN(RAM) + LENGTH(RAM);  /* Explicit stack top for Cortex-M */

SECTIONS
{
  /* Interrupt Vector Table (256B aligned) */
  .isr_vector :
  {
    . = ALIGN(256);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } > BOOTLOADER

  /* Code and RO Data */
  .text :
  {
    *(.text .text* .glue_7* .eh_frame)
    KEEP(*(.init)) KEEP(*(.fini))
    . = ALIGN(4);
    _etext = .;
  } > BOOTLOADER

  .rodata : { *(.rodata .rodata*) } > BOOTLOADER

  /* ARM Exception Handling */
  .ARM.extab : { *(.ARM.extab*) } > BOOTLOADER
  .ARM.exidx :
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } > BOOTLOADER

  /* C++ Constructors/Destructors */
  .preinit_array :
  {
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array*))
    PROVIDE_HIDDEN(__preinit_array_end = .);
  } > BOOTLOADER

  .init_array :
  {
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT(.init_array.*))) KEEP(*(.init_array*))
    PROVIDE_HIDDEN(__init_array_end = .);
  } > BOOTLOADER

  /* Initialized Data (copied to RAM at startup) */
  _sidata = LOADADDR(.data);
  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data .data*)
    *(.RamFunc*)        /* Explicit placement of RAM functions */
    . = ALIGN(4);
    _edata = .;
  } > RAM AT > BOOTLOADER

  /* Uninitialized Data (BSS) */
  .bss :
  {
    . = ALIGN(4);
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss .bss* COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } > RAM

  /* Persistent Data (survives soft reset) */
  .noinit (NOLOAD) :
  {
    *(.noinit*)
  } > RAM

  /* Heap (grows upward) */
  ._user_heap :
  {
    . = ALIGN(8);
    PROVIDE(end = .);
    PROVIDE(_end = .);
    __heap_start__ = .;
    __heap_end__   = _estack - _Min_Stack_Size;
    ASSERT(__heap_end__ > __heap_start__, "Heap too large, would collide with stack!");
  } > RAM

  /* Stack (at top of RAM, grows downward) */
  ._user_stack :
  {
    . = ALIGN(8);
    __stack_limit__ = _estack - _Min_Stack_Size;
    ASSERT(__stack_limit__ >= _ebss, "Stack collides with heap/BSS!");
    . = _estack;
  } > RAM

  /* Core-Coupled RAM (DMA buffers, etc.) */
  .ram2_section (NOLOAD) :
  {
    . = ALIGN(4);
    *(.ram2_buffer*)    /* Explicitly named section for DMA/HSRAM */
    *(.ram2_section*)
    . = ALIGN(4);
  } > RAM2

  /* Boot Flags in Backup RAM (persists across resets) */
  .boot_flags (NOLOAD) :
  {
    KEEP(*(.boot_flags))  /* Use this section in code for flags */
  } > BKPSRAM

  /* Discard Unnecessary Sections */
  /DISCARD/ : { *(.comment) *(.note*) }
}