/*
 * GCC linker script (flash) for PIC32CM5164JH01048
 *
 * Copyright (c) 2026 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

/*------------------------------------------------------------------------------
 *      Linker script for running in internal FLASH on the PIC32CM5164JH01048
 *----------------------------------------------------------------------------*/

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
SEARCH_DIR(.)

/*************************************************************************
 * Memory-Region Definitions
 * You may define these as options to the linker when calling
 * the linker via the arm-gcc shell.
 * ie. '-Wl,--defsym=<memory_type>_ORIGIN=<address>'
 * and/or '-Wl,--defsym=<memory_type>_LENGTH=<size>'
 *************************************************************************/

/*---------------------- ROM Configuration ----------------------------------
  <h> ROM Configuration
    <o0> ROM Base Address <0x0-0xFFFFFFFF:8>
    <o1> ROM Size (in Bytes) <0x0-0xFFFFFFFF:8>
  </h>
  -----------------------------------------------------------------------------*/
ROM_ORIGIN = DEFINED(ROM_ORIGIN) ? ROM_ORIGIN : 0x00000000;
ROM_LENGTH = DEFINED(ROM_LENGTH) ? ROM_LENGTH : 0x80000; /* rom, 524288K */
/*---------------------- RAM Configuration ----------------------------------
  <h> RAM Configuration
    <o0> RAM Base Address <0x0-0xFFFFFFFF:8>
    <o1> RAM Size (in Bytes) <0x0-0xFFFFFFFF:8>
  </h>
  -----------------------------------------------------------------------------*/
RAM_ORIGIN = DEFINED(RAM_ORIGIN) ? RAM_ORIGIN : 0x20000000;
RAM_LENGTH = DEFINED(RAM_LENGTH) ? RAM_LENGTH : 0x10000; /* ram, 65536K */

/* The stack and heap size used by the application. NOTE: you need to adjust according to your application. */
/*--------------------- Stack / Heap Configuration ----------------------------
  <h> Stack / Heap Configuration
    <o0> Stack Size (in Bytes) <0x0-0xFFFFFFFF:8>
    <o1> Heap Size (in Bytes) <0x0-0xFFFFFFFF:8>
  </h>
  -----------------------------------------------------------------------------*/
STACK_SIZE = DEFINED(STACK_SIZE) ? STACK_SIZE : DEFINED(__stack_size__) ? __stack_size__ : 0x0400;
HEAP_SIZE = DEFINED(HEAP_SIZE) ? HEAP_SIZE : DEFINED(__heap_size__) ? __heap_size__ : 0x0200;

/* ARMv8-M stack sealing:
   to use ARMv8-M stack sealing set __STACKSEAL_SIZE to 8 otherwise keep 0
 */
__STACKSEAL_SIZE = DEFINED(__STACKSEAL_SIZE) ? __STACKSEAL_SIZE : 0;

/*************************************************************************
 * Memory-Region Definitions
 * The MEMORY command describes the location and size of blocks of memory
 * on the target device. The command below uses the macros defined above.
 *************************************************************************/
MEMORY
{
    rom (rx)    : ORIGIN = ROM_ORIGIN, LENGTH = ROM_LENGTH
    ram (rwx)   : ORIGIN = RAM_ORIGIN, LENGTH = RAM_LENGTH
    /* Device configuration memory regions (DEVXML-7677) */
    /* Use __attribute__((section(".<section_name>"))) to place data in these regions */
    cal                  (rw!x)  : ORIGIN = 0x00800000, LENGTH = 0x8
    lockbit              (rw!x)  : ORIGIN = 0x00802000, LENGTH = 0x4
    user_page            (rw!x)  : ORIGIN = 0x00804000, LENGTH = 0x100
    otp1                 (rw!x)  : ORIGIN = 0x00806000, LENGTH = 0x8
    otp2                 (rw!x)  : ORIGIN = 0x00806008, LENGTH = 0x8
    otp3                 (rw!x)  : ORIGIN = 0x00806010, LENGTH = 0x8
    otp4                 (rw!x)  : ORIGIN = 0x00806018, LENGTH = 0x8
    otp5                 (rw!x)  : ORIGIN = 0x00806020, LENGTH = 0x8
    temp_log             (rw!x)  : ORIGIN = 0x00806030, LENGTH = 0x8
}

ENTRY(Reset_Handler)

/*************************************************************************
 * ELF Program Header Declarations (PHDRS)
 *
 * Without an explicit PHDRS block the linker auto-generates PT_LOAD
 * segments.  Because .relocate has its LMA in flash (AT _etext) but its
 * VMA in RAM, the auto-generated segment would span both the executable
 * code and the writable data initialiser image, resulting in a single
 * PT_LOAD with R+W+X permissions.  GNU ld 2.39 / GCC 12 and later emit:
 *
 *   warning: <elf> has a LOAD segment with RWX permissions
 *
 * The fix is to declare two explicit PT_LOAD segments with precise flags:
 *
 *   flash_rx  — read + execute (FLAGS = R|X = 5)
 *               Covers all code and read-only data that lives in flash.
 *               Sections: .text, .ARM.exidx, .copy.table, .zero.table
 *
 *   flash_rw  — read + write  (FLAGS = R|W = 6)
 *               Covers the initialised-data image that is stored in flash
 *               (LMA) but copied to RAM at startup (VMA).
 *               Section: .relocate
 *
 * NOLOAD sections (.bss, .heap, .stack) are not part of any LOAD segment
 * and therefore carry no phdr assignment — the linker omits them from the
 * ELF program-header table automatically.
 *
 * Note: lld (LLVM linker) also supports the PHDRS syntax, so this fix is
 * compatible with both GNU ld and Clang/lld toolchains.
 *************************************************************************/
PHDRS
{
    flash_rx  PT_LOAD FLAGS(5);  /* R(4) + X(1) — executable code in flash    */
    flash_rw  PT_LOAD FLAGS(6);  /* R(4) + W(2) — initialised data init image */
}

/* Section Definitions */
SECTIONS
{
    .text :
    {
        . = ALIGN(4);
        _sfixed = .;
        KEEP(*(.vectors .vectors.*))
        *(.text .text.* .gnu.linkonce.t.*)
        *(.glue_7t) *(.glue_7)
        *(.rodata .rodata* .gnu.linkonce.r.*)
        *(.ARM.extab* .gnu.linkonce.armextab.*)

        /* Support C constructors, and C destructors in both user code
           and the C library. This also provides support for C++ code. */
        . = ALIGN(4);
        KEEP(*(.init))
        . = ALIGN(4);
        __preinit_array_start = .;
        KEEP (*(.preinit_array))
        __preinit_array_end = .;

        . = ALIGN(4);
        __init_array_start = .;
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array))
        __init_array_end = .;

        . = ALIGN(4);
        KEEP (*crtbegin.o(.ctors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*crtend.o(.ctors))

        . = ALIGN(4);
        KEEP(*(.fini))

        . = ALIGN(4);
        __fini_array_start = .;
        KEEP (*(.fini_array))
        KEEP (*(SORT(.fini_array.*)))
        __fini_array_end = .;

        KEEP (*crtbegin.o(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*crtend.o(.dtors))

        . = ALIGN(4);
        _efixed = .;            /* End of text section */
    } > rom  :flash_rx

    /* .ARM.exidx is sorted, so has to go in its own output section.  */
    PROVIDE_HIDDEN (__exidx_start = .);
    .ARM.exidx :
    {
      *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > rom  :flash_rx
    PROVIDE_HIDDEN (__exidx_end = .);

    .copy.table :
    {
        . = ALIGN(4);
        __copy_table_start__ = .;

        LONG (LOADADDR(.relocate))
        LONG (ADDR(.relocate))
        LONG (SIZEOF(.relocate) / 4)

        /* Add each additional data section here */
        /*
            LONG (LOADADDR(.data2))
            LONG (ADDR(.data2))
            LONG (SIZEOF(.data2) / 4)
        */
        __copy_table_end__ = .;
    } > rom  :flash_rx

    .zero.table :
    {
        . = ALIGN(4);
        __zero_table_start__ = .;

        /*  .bss initialization to zero is already done during C Run-Time Startup.
        LONG (ADDR(.bss))
        LONG (SIZEOF(.bss) / 4)
        */

        /* Add each additional bss section here */
        /*
            LONG (ADDR(.bss2))
            LONG (SIZEOF(.bss2) / 4)
        */
        __zero_table_end__ = .;
    } > rom  :flash_rx

    . = ALIGN(4);
    /* _etext  : newlib crt0/crt1 uses this as the LMA source address from
     *           which the .data section is copied into RAM at startup.
     * __etext : alias used by some CMSIS-style startup files. */
    _etext = .;
    __etext = .;

    .relocate : AT (_etext)
    {
        . = ALIGN(4);
        /* _srelocate / _erelocate : legacy Microchip/ASF startup files
         *   (startup_*.c shipped in older Atmel Software Framework packs)
         *   iterate between these two symbols to copy .data from flash. */
        _srelocate = .;
        /* __data_start__ / __data_end__ : ARM CMSIS startup and
         *   newlib-nano (nano.specs) startup copy .data between these. */
        __data_start__ = .;
        *(vtable);
        *(.ramfunc .ramfunc.*);
        *(.data .data.*);
        . = ALIGN(4);
        _erelocate = .;
        __data_end__ = .;
    } > ram  :flash_rw

    /* Picolibc / Clang runtime symbol aliases
     *
     * The DFP maps both GCC and CLANG to these linker scripts via the shared
     * GCC+CLANG condition.  The LLVM embedded toolchain ships picolibc as its
     * C runtime instead of newlib.  Picolibc's startup file (crt0.c) and its
     * TLS helper (set_tls.c) expect symbol names that differ from the GNU ld
     * double-underscore convention defined above.  PROVIDE is used so each
     * alias is only emitted when the name is not already defined elsewhere.
     *
     * .data aliases — referenced by picolibc crt0.c:_start():
     *   __data_start  : VMA (run address) of the first byte of .data.
     *                   GNU ld equivalent: __data_start__
     *   __data_source : LMA (load address, i.e. flash address) from which
     *                   .data is copied at startup.
     *                   GNU ld equivalent: _etext / LOADADDR(.relocate)
     *   __data_size   : Byte length of .data to copy.
     *                   GNU ld equivalent: __data_end__ - __data_start__ */
    PROVIDE(__data_start  = __data_start__);
    PROVIDE(__data_source = LOADADDR(.relocate));
    PROVIDE(__data_size   = __data_end__ - __data_start__);

    /* .bss section which is used for uninitialized data */
    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        /* _sbss / _ebss and _szero / _ezero : legacy Microchip/ASF startup
         *   files zero-initialise .bss between these symbols.  Both pairs
         *   refer to the same addresses; _szero/_ezero is an older naming
         *   convention found in pre-2015 Atmel START / ASF packs. */
        _sbss = . ;
        _szero = .;
        /* __bss_start__ / __bss_end__ : ARM CMSIS startup and newlib-nano
         *   startup zero-initialise .bss between these symbols. */
        __bss_start__ = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = . ;
        _ezero = .;
        __bss_end__ = .;
    } > ram

    /* .bss aliases — referenced by picolibc crt0.c:_start():
     *   __bss_start : VMA of the first byte of .bss.
     *                 GNU ld equivalent: __bss_start__
     *   __bss_size  : Byte length of .bss to zero-initialise.
     *                 GNU ld equivalent: __bss_end__ - __bss_start__ */
    PROVIDE(__bss_start = __bss_start__);
    PROVIDE(__bss_size  = __bss_end__ - __bss_start__);

    /* TLS aliases — referenced by picolibc set_tls.c:_set_tls():
     *   __tls_base            : Base address of the TLS block.  Bare-metal
     *                           targets have no dynamic TLS, so 0 is a safe
     *                           default; the application can override it by
     *                           defining the symbol before this PROVIDE.
     *   __arm32_tls_tcb_offset: Byte offset from the TLS pointer to the
     *                           Thread Control Block on ARM32 (defined by
     *                           the ARM ELF ABI as 8 bytes). */
    PROVIDE(__tls_base            = 0);
    PROVIDE(__arm32_tls_tcb_offset = 8);

    /* heap section */
    .heap (NOLOAD):
    {
        . = ALIGN(8);
        /* _sheap / _eheap : legacy Microchip/ASF startup files use these to
         *   track the heap region boundaries. */
        _sheap = .;
        /* __end__ : newlib's sbrk() implementation uses this as the initial
         *   heap pointer when growing the heap upward.
         * end      : the standard C symbol for the start of the heap; also
         *   consumed by newlib's sbrk() and by some C++ operator new
         *   implementations. */
        __end__ = .;
        PROVIDE(end = .);
        . = . + HEAP_SIZE;
        . = ALIGN(8);
        _eheap = .;
        /* __HeapLimit : ARM CMSIS startup checks that the heap does not
         *   collide with the stack using this symbol. */
        __HeapLimit = .;
    } > ram

    /* stack section */
    .stack (NOLOAD):
    {
        . = ALIGN(8);
        /* _sstack / _estack : legacy Microchip/ASF startup files reference
         *   these to locate the stack region. */
        _sstack = .;
        /* __StackLimit / __StackTop : ARM CMSIS startup initialises the
         *   Main Stack Pointer to __StackTop and uses __StackLimit for
         *   optional stack-overflow checking (e.g. MSPLIM on ARMv8-M). */
        __StackLimit = .;
        . = . + STACK_SIZE;
        . = ALIGN(8);
        _estack = .;
        __StackTop = .;
    } > ram

    /* __stack : newlib's _init_signal and some Microchip startup files use
     *   this symbol to initialise the stack pointer.  PROVIDE means it is
     *   only emitted when not already defined by the application. */
    PROVIDE(__stack = __StackTop);

    . = ALIGN(4);
    /* _end : legacy symbol — equivalent to end / __end__; retained for
     *   compatibility with older Microchip/ASF startup and sbrk stubs. */
    _end = . ;
    _ram_end_ = ORIGIN(ram) + LENGTH(ram) - 1 ;

    /* Device configuration sections */
    /* Use __attribute__((section(".<section_name>"))) to place data in these regions */
    .cal (NOLOAD) :
    {
        KEEP(*(.cal .cal.*))
    } > cal
    .lockbit (NOLOAD) :
    {
        KEEP(*(.lockbit .lockbit.*))
    } > lockbit
    .user_page (NOLOAD) :
    {
        KEEP(*(.user_page .user_page.*))
    } > user_page
    .otp1 (NOLOAD) :
    {
        KEEP(*(.otp1 .otp1.*))
    } > otp1
    .otp2 (NOLOAD) :
    {
        KEEP(*(.otp2 .otp2.*))
    } > otp2
    .otp3 (NOLOAD) :
    {
        KEEP(*(.otp3 .otp3.*))
    } > otp3
    .otp4 (NOLOAD) :
    {
        KEEP(*(.otp4 .otp4.*))
    } > otp4
    .otp5 (NOLOAD) :
    {
        KEEP(*(.otp5 .otp5.*))
    } > otp5
    .temp_log (NOLOAD) :
    {
        KEEP(*(.temp_log .temp_log.*))
    } > temp_log

  /* Check if data + heap + stack exceeds RAM limit */
  ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack")
}
