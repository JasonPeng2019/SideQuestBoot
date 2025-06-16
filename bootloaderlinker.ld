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
  BOOTLOADER (rx)  : ORIGIN = 0x08000000, LENGTH = 32K
  MAIN_APP   (rx)  : ORIGIN = 0x08008000, LENGTH = 200K
  BACKUP_APP (rx)  : ORIGIN = 0x08040000, LENGTH = 200K
  UPDATE_IMG (rx)  : ORIGIN = 0x08080000, LENGTH = 200K
  B1_FLAGS   (rw)  : ORIGIN = 0x080C0000, LENGTH = 8K      /* Removed - using backup RAM instead */
  B2_FLAGS   (rw)  : ORIGIN = 0x080C2000, LENGTH = 8K      /* Removed - using backup RAM instead */
  RAM        (xrw) : ORIGIN = 0x20000000, LENGTH = 96K
  RAM2       (xrw) : ORIGIN = 0x10000000, LENGTH = 32K
  BKPSRAM    (rw)  : ORIGIN = 0x40024000, LENGTH = 4K      /* Added for persistent boot flags */
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