/*
******************************************************************************
**
** @file        : bootloaderlinker_bank1.ld
**
** @author      : buh07
** @Collaborators: Jason.p
**
**  Abstract    : Linker script for NUCLEO-L476RG Board embedding STM32L476RGTx Device
**                placing Bootloader, Update Image, and Stable App partitions
**                in Flash Bank 1 (0x08000000 – 0x0807FFFF).
**
**                1024 KB FLASH total (Bank1 = first 512 KB at 0x08000000; Bank2 = next 512 KB).
**                96 KB RAM, 32 KB RAM2.
**
**                Set heap size, stack size and stack location according
**                to application requirements for each build.
**
******************************************************************************/

/* Entry Point for Bootloader build */
ENTRY(Reset_Handler)

/* Stack pointer initialization (in RAM) */
_estack = ORIGIN(RAM) + LENGTH(RAM);

/* Heap and Stack sizes */
_Min_Heap_Size = 0x200;    /* 512 Bytes */
_Min_Stack_Size = 0x400;   /* 1 KB */

/* === MEMORY REGIONS === */
MEMORY
{
  /* === FLASH BANK 1: 0x0800_0000 – 0x0807_FFFF (512 KB) === */
  /* Partition Bootloader: first 32 KB */
  BOOTLOADER   (rx)  : ORIGIN = 0x08000000, LENGTH = 32K

  /* Partition Update Image: next 240 KB */
  UPDATE_IMG   (rx)  : ORIGIN = 0x08008000, LENGTH = 240K

  /* Partition Stable App: following Update Image, 240 KB */
  STABLE_APP   (rx)  : ORIGIN = 0x08044000, LENGTH = 240K

  /* === SRAM === */
  RAM          (xrw) : ORIGIN = 0x20000000, LENGTH = 96K     /* Main SRAM */
  RAM2         (xrw) : ORIGIN = 0x10000000, LENGTH = 32K     /* Additional SRAM (e.g. DTCM or Backup RAM) */
}

/* Sections */
SECTIONS
{
  /* Exported symbols for partitions & RAM */
  __bootloader_start__   = ORIGIN(BOOTLOADER);
  __bootloader_size__    = LENGTH(BOOTLOADER);

  __update_img_start__   = ORIGIN(UPDATE_IMG);
  __update_img_size__    = LENGTH(UPDATE_IMG);

  __stable_app_start__   = ORIGIN(STABLE_APP);
  __stable_app_size__    = LENGTH(STABLE_APP);

  __ram_start__          = ORIGIN(RAM);
  __ram_size__           = LENGTH(RAM);
  __ram2_start__         = ORIGIN(RAM2);
  __ram2_size__          = LENGTH(RAM2);

  /* ===== BOOTLOADER region sections =====
     When building the Bootloader, direct code/data here.
     For Update or Stable App builds, adapt to direct into UPDATE_IMG or STABLE_APP.
  */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector)) /* Vector table for Bootloader */
    . = ALIGN(4);
  } >BOOTLOADER

  .text :
  {
    . = ALIGN(4);
    *(.text)           /* Code */
    *(.text*)          /* Additional text sections */
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;
  } >BOOTLOADER

  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } >BOOTLOADER

  .ARM.extab :
  {
    . = ALIGN(4);
    *(.ARM.extab* .gnu.linkonce.armextab.*)
    . = ALIGN(4);
  } >BOOTLOADER

  .ARM :
  {
    . = ALIGN(4);
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
    . = ALIGN(4);
  } >BOOTLOADER

  .preinit_array :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
    . = ALIGN(4);
  } >BOOTLOADER

  .init_array :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
    . = ALIGN(4);
  } >BOOTLOADER

  .fini_array :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
    . = ALIGN(4);
  } >BOOTLOADER

  /* Data load address for .data in RAM */
  _sidata = LOADADDR(.data);

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    *(.RamFunc)
    *(.RamFunc*)
    . = ALIGN(4);
    _edata = .;
  } >RAM AT> BOOTLOADER

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

  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM

  /* Discard standard libs if linked in inadvertently; Bootloader should avoid heavy libs */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}

/* 
  === How to adapt for Update Image or Stable App builds ===

  - For Update Image build:
    • Change ENTRY(…) if needed (entry is Reset_Handler in update image).
    • In MEMORY, direct FLASH to only UPDATE_IMG region:
        MEMORY {
          UPDATE_IMG (rx) : ORIGIN = 0x08008000, LENGTH = 240K
          RAM        (xrw): ORIGIN = 0x20000000, LENGTH = 96K
          RAM2       (xrw): ORIGIN = 0x10000000, LENGTH = 32K
        }
    • In SECTIONS, replace all `>BOOTLOADER` with `>UPDATE_IMG`, and vector table section uses KEEP(*(.isr_vector)) so vector table is at 0x08008000.
    • The application’s startup code will copy .data from LOADADDR (flash) into RAM as usual.

  - For Stable App build:
    • Similarly, MEMORY with STABLE_APP region:
        MEMORY {
          STABLE_APP (rx) : ORIGIN = 0x08044000, LENGTH = 240K
          RAM and RAM2 unchanged
        }
    • SECTIONS: direct to STABLE_APP.
    • ENTRY is Reset_Handler in stable application; vector table placed at 0x08044000.

  - You may maintain three separate linker scripts (e.g., `bootloader_bank1.ld`, `update_bank1.ld`, `stable_bank1.ld`), each with the appropriate MEMORY/SECTIONS settings. This is the cleanest approach.
*/

/* End of linker script */
