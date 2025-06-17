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

/* Entry Point */
ENTRY(Reset_Handler)

/* Define end of RAM to initialize stack pointer */
_estack = ORIGIN(RAM) + LENGTH(RAM);

/* Define minimal sizes for heap and stack */
_Min_Heap_Size = 0x200;     /* 512 Bytes heap */
_Min_Stack_Size = 0x400;    /* 1 KB stack */

/* === Memory Regions Definition === */
/* MEMORY regions definition for STM32L476RG dual-bank system */

MEMORY
{
  /* === FLASH BANK 1 === */
  /* MAIN_APPLICATION occupies the full 512 KB of Bank 1 */
  MAIN_APP   (rx)  : ORIGIN = 0x08000000, LENGTH = 512K

  /* === FLASH BANK 2 === */
  /* BOOTLOADER occupies the first 32 KB of Bank 2 */
  BOOTLOADER (rw)  : ORIGIN = 0x08080000, LENGTH = 32K

  /* UPDATE_IMG follows BOOTLOADER (32K offset), 240 KB */
  UPDATE_IMG (rw)  : ORIGIN = 0x08088000, LENGTH = 240K

  /* BACKUP_APP follows UPDATE_IMG (240K offset), 240 KB */
  BACKUP_APP (rw)  : ORIGIN = 0x080C8000, LENGTH = 240K

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


/* ******************************************************************************
**
** @file        : mainlinker.ld
**
** @brief       : Linker script for MAIN_APP on STM32L476RG (Bank 1)
**
**                Flash layout:
**                - MAIN_APP     : 512 KB @ 0x08000000 (Bank 1)
**                - BOOTLOADER   :  32 KB @ 0x08080000 (Bank 2)
**                - UPDATE_IMG   : 240 KB @ 0x08088000 (Bank 2)
**                - BACKUP_APP   : 240 KB @ 0x080C8000 (Bank 2)
**
**                RAM:
**                - RAM          : 96 KB @ 0x20000000
**                - RAM2         : 32 KB @ 0x10000000
**
******************************************************************************/

/* Entry Point */
ENTRY(Reset_Handler)

/* Stack pointer initialization */
_estack = ORIGIN(RAM) + LENGTH(RAM);

/* Heap and Stack sizes */
_Min_Heap_Size = 0x200;    /* 512 Bytes */
_Min_Stack_Size = 0x400;   /* 1 KB */

/* === MEMORY REGIONS === */
MEMORY
{
  /* === FLASH BANK 1 === */
  /* MAIN_APPLICATION occupies the full 512 KB of Bank 1 */
  MAIN_APP   (rx)  : ORIGIN = 0x08000000, LENGTH = 512K

  /* === FLASH BANK 2 === */
  /* BOOTLOADER occupies the first 32 KB of Bank 2 */
  BOOTLOADER (rw)  : ORIGIN = 0x08080000, LENGTH = 32K

  /* UPDATE_IMG follows BOOTLOADER (32K offset), 240 KB */
  UPDATE_IMG (rw)  : ORIGIN = 0x08088000, LENGTH = 240K

  /* BACKUP_APP follows UPDATE_IMG (240K offset), 240 KB */
  BACKUP_APP (rw)  : ORIGIN = 0x080C8000, LENGTH = 240K

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

/* === SECTIONS === */
SECTIONS
{
  /* Exported symbols for memory regions */
  __main_app_start__     = ORIGIN(MAIN_APP);
  __main_app_end__       = ORIGIN(MAIN_APP) + LENGTH(MAIN_APP);
  __main_app_size__      = LENGTH(MAIN_APP);

  __bootloader_start__   = ORIGIN(BOOTLOADER);
  __bootloader_size__    = LENGTH(BOOTLOADER);

  __update_img_start__   = ORIGIN(UPDATE_IMG);
  __update_img_size__    = LENGTH(UPDATE_IMG);

  __backup_app_start__   = ORIGIN(BACKUP_APP);
  __backup_app_size__    = LENGTH(BACKUP_APP);

  __ram_start__          = ORIGIN(RAM);
  __ram_size__           = LENGTH(RAM);
  __ram2_start__         = ORIGIN(RAM2);
  __ram2_size__          = LENGTH(RAM2);

  /* Interrupt vector table */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >MAIN_APP

  /* Application code */
  .text :
  {
    . = ALIGN(4);
    _stext = .;
    *(.text)
    *(.text*)
    *(.glue_7) *(.glue_7t)
    *(.eh_frame)
    KEEP (*(.init))
    KEEP (*(.fini))
    . = ALIGN(4);
    _etext = .;
  } >MAIN_APP

  /* Read-only constants */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } >MAIN_APP

  /* Exception handling tables */
  .ARM.extab (READONLY) :
  {
    . = ALIGN(4);
    *(.ARM.extab* .gnu.linkonce.armextab.*)
    . = ALIGN(4);
  } >MAIN_APP

  .ARM (READONLY) :
  {
    . = ALIGN(4);
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
    . = ALIGN(4);
  } >MAIN_APP

  /* C++ runtime support */
  .preinit_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
    . = ALIGN(4);
  } >MAIN_APP

  .init_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
    . = ALIGN(4);
  } >MAIN_APP

  .fini_array (READONLY) :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
    . = ALIGN(4);
  } >MAIN_APP

  /* Data segment (copied from FLASH to RAM) */
  _sidata = LOADADDR(.data);

  .data : AT (_sidata)
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    *(.RamFunc)
    *(.RamFunc*)
    . = ALIGN(4);
    _edata = .;
  } >RAM

  /* BSS segment */
  .bss :
  {
    . = ALIGN(4);
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } >RAM

  /* Heap and stack region */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM

  /* Remove standard libs */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  /* ARM-specific metadata */
  .ARM.attributes 0 : { *(.ARM.attributes) }
}