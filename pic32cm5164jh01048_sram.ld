/*
 * GCC linker script (sram) for PIC32CM5164JH01048
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
 *      Linker script for running in internal SRAM on the PIC32CM5164JH01048
 *----------------------------------------------------------------------------*/

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
SEARCH_DIR(.)

/* Memory Spaces Definitions */
MEMORY
{
    rom (rx)    : ORIGIN = 0x00000000, LENGTH = 0x00080000 /* rom, 524288K */
    ram (rwx)   : ORIGIN = 0x20000000, LENGTH = 0x00010000 /* ram, 65536K */
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
    } > ram

    . = ALIGN(4);
    /* _etext  : newlib crt0/crt1 uses this as the LMA source address from
     *           which the .data section is copied into RAM at startup.  In
     *           SRAM mode LMA == VMA so no actual copy occurs, but the symbol
     *           must still be defined for compatibility. */
    _etext = .;

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
    } > ram

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
     *   __data_source : LMA (load address) from which .data is copied at
     *                   startup.  In SRAM mode this equals __data_start
     *                   because code and data both reside in RAM, but the
     *                   expression is kept consistent with the flash variant.
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

    /* .ARM.exidx is sorted, so has to go in its own output section.  */
    PROVIDE_HIDDEN (__exidx_start = .);
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > ram
    PROVIDE_HIDDEN (__exidx_end = .);

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

    . = ALIGN(4);
    _end = . ;
    _ram_end_ = ORIGIN(ram) + LENGTH(ram) - 1 ;
}
