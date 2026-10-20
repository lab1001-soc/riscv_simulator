# RISC-V UART Demo Makefile
# Builds a bare-metal RISC-V binary for the RV_sim simulator

ARCH     = -march=rv32imafc -mabi=ilp32f
GCC_PATH = C:\riscv_gcc
PREFIX   = $(GCC_PATH)\bin\riscv32-unknown-elf

OBJS = 	../../libraries/startup/startup  \
		../../libraries/core/MyPrintf  \
		../../libraries/sdk/uart  \
		../../libraries/sdk/mtimer  \
		../../libraries/sdk/software  \
        main

OBJS += rtthread-nano/rt-thread/bsp/rv_sim/board \

OBJS += rtthread-nano/rt-thread/components/finsh/cmd \
		rtthread-nano/rt-thread/components/finsh/msh_file \
		rtthread-nano/rt-thread/components/finsh/msh_parse \
		rtthread-nano/rt-thread/components/finsh/msh \
		rtthread-nano/rt-thread/components/finsh/shell \

OBJS += rtthread-nano/rt-thread/libcpu/risc-v/common/context_gcc \
		rtthread-nano/rt-thread/libcpu/risc-v/common/cpuport \

OBJS += rtthread-nano/rt-thread/libcpu/risc-v/rv_sim/interrupt_gcc \
		rtthread-nano/rt-thread/libcpu/risc-v/rv_sim/cpuport_interrupt \

OBJS += rtthread-nano/rt-thread/src/clock \
		rtthread-nano/rt-thread/src/components \
		rtthread-nano/rt-thread/src/cpu \
		rtthread-nano/rt-thread/src/device \
		rtthread-nano/rt-thread/src/idle \
		rtthread-nano/rt-thread/src/ipc \
		rtthread-nano/rt-thread/src/irq \
		rtthread-nano/rt-thread/src/kservice \
		rtthread-nano/rt-thread/src/mem \
		rtthread-nano/rt-thread/src/memheap \
		rtthread-nano/rt-thread/src/mempool \
		rtthread-nano/rt-thread/src/object \
		rtthread-nano/rt-thread/src/scheduler \
		rtthread-nano/rt-thread/src/signal \
		rtthread-nano/rt-thread/src/slab \
		rtthread-nano/rt-thread/src/thread \
		rtthread-nano/rt-thread/src/timer \


INC_DIR  = -I ./
INC_DIR += -I ../../libraries/core
INC_DIR += -I ../../libraries/sdk
INC_DIR += -I rtthread-nano/rt-thread/bsp/rv_sim
INC_DIR += -I rtthread-nano/rt-thread/components/finsh
INC_DIR += -I rtthread-nano/rt-thread/include
INC_DIR += -I rtthread-nano/rt-thread/libcpu/risc-v/common

DEF = -DRTOS_RTTHREAD

CFLAG  = -c -g -Os $(INC_DIR) $(DEF) -msmall-data-limit=0 -fno-toplevel-reorder \
         -ffunction-sections -fdata-sections $(ARCH) -std=gnu99

LDFLAG = -T output/ld.script -nostartfiles -Xlinker --gc-section \
         -Wl,-Map,"output/memmap" \
         -L $(GCC_PATH)/riscv32-unknown-elf/lib/ \
         -L $(GCC_PATH)/lib/gcc/riscv32-unknown-elf/8.3.0/ \
         -lgcc -lc -lm


# --------------------------------------------------------------------------
# Inline linker script
# --------------------------------------------------------------------------
define ldscript =
OUTPUT_ARCH( "riscv" )

MEMORY
{
    flash         (rxai!w) : ORIGIN = 0x01000000, LENGTH = 512K
    ram           (wxa!ri) : ORIGIN = 0x10060, LENGTH = 0x100000-0x10060
    ramcode       (rxai!w) : ORIGIN = 0x100000, LENGTH = 64K
}

SECTIONS
{
	__stack_size = DEFINED(__stack_size) ? __stack_size : 24K;
	.text           :
    {
		KEEP(*startup*)
        *(.text*)
        *(.gnu.linkonce.t.*)
    } >flash AT>flash

	.rodata : ALIGN(4)
    {
        . = ALIGN(4);
        *(.rdata*)
        *(.rodata .rodata.*)
		*(.eh_frame*)
		*(.srodata*)
		*(.srodata.cst*)

        /* section information for initial. */
        . = ALIGN(4);
        __rt_init_start = .;
        KEEP(*(SORT(.rti_fn*)))
        __rt_init_end = .;
        /* section information for finsh shell */
        . = ALIGN(4);
        __fsymtab_start = .;
        KEEP(*(FSymTab))
        __fsymtab_end = .;
        . = ALIGN(4);
        __vsymtab_start = .;
        KEEP(*(VSymTab))
        __vsymtab_end = .;
        *(.gnu.linkonce.r.*)

    } >flash AT>flash

	.fini           :
    {
        KEEP (*(SORT_NONE(.fini)))
    } >flash AT>flash

    . = ALIGN(4);

    .ctors          :
    {
        /* gcc uses crtbegin.o to find the start of
        the constructors, so we make sure it is
        first.  Because this is a wildcard, it
        doesn't matter if the user does not
        actually link against crtbegin.o; the
        linker won't look for a file to match a
        wildcard.  The wildcard also means that it
        doesn't matter which directory crtbegin.o
        is in.  */
        KEEP (*crtbegin.o(.ctors))
        KEEP (*crtbegin?.o(.ctors))
        /* We don't want to include the .ctor section from
        the crtend.o file until after the sorted ctors.
        The .ctor section from the crtend file contains the
        end of ctors marker and it must be last */
        KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*(.ctors))
    } >flash AT>flash

    .dtors          :
    {
        KEEP (*crtbegin.o(.dtors))
        KEEP (*crtbegin?.o(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*(.dtors))
    } >flash AT>flash

    . = ALIGN(4);
	PROVIDE( _sidata = . );
	.data          :
    {
        . = ALIGN(4);
        PROVIDE(_sdata = .);
        *(.data .data.*)
        *(.gnu.linkonce.d.*)

        . = ALIGN(8);
		PROVIDE( _gp = . + 0x800 );
        *(.sdata .sdata.* .sdata*)
        *(.gnu.linkonce.s.*)

        . = ALIGN(4);
        PROVIDE( _edata = . );
        PROVIDE( edata = . );
    } >ram AT>flash

	.bss : {
		PROVIDE( _sbss = . );
		*(.sbss*)
		*(.gnu.linkonce.sb.*)
		*(.bss .bss.*)
		*(COMMON)
		*(*.bss)
		*(*.scommon*)
		PROVIDE( _ebss = . );
	}>ram

	. = ALIGN(8);
	_end = .;
	end = .;
	. = 0x100000 - __stack_size;
	_heap_end = . ;
	. = . + __stack_size;
	_stack = .;
	_sp = .;

	_siitcm = _sidata + _edata - _sdata;
	. = _stack;
	.textram : AT (_siitcm) {_sitcm = .; *(._SECTION_RAM_CODE_DEFINE*) _eitcm = .; }
	ASSERT(_ebss < _stack - __stack_size, "data size overflow")
}
endef

# --------------------------------------------------------------------------
# Perl reformatter for disassembly output
# --------------------------------------------------------------------------
define addspace =
while(<stdin>) {
s/(.+:\s+[0-9a-f]{8})\s+/\1            \t/;
print $$_;
}
endef

# --------------------------------------------------------------------------
# Default target
# --------------------------------------------------------------------------
all : output/obj output/a.out

# --------------------------------------------------------------------------
# Link step + post-processing
# --------------------------------------------------------------------------
output/a.out : $(addprefix output/,$(addsuffix .o,$(OBJS)))
	$(file > output/ld.script, $(ldscript))
	$(file > output/tmp.pl, $(addspace))
	$(PREFIX)-gcc $(addprefix output/obj/,$(addsuffix .o, $(notdir $(OBJS)))) -o $@ $(LDFLAG)
	$(PREFIX)-objdump -S $@ | perl output/tmp.pl > output/disasm1
	$(PREFIX)-objcopy $@ -O ihex output/out.hex
	../../util/f_hextobin 1000000 1ffffff output/out.hex output/out.bin

# --------------------------------------------------------------------------
# Ensure output directories exist
# --------------------------------------------------------------------------
output/obj:
	mkdir output\obj

# --------------------------------------------------------------------------
# Compile rules - explicit since sources live in different directories
# --------------------------------------------------------------------------
output/%.o : %.S
	$(PREFIX)-gcc $^ -o output/obj/$(notdir $@) $(CFLAG)

output/%.o : %.c
	$(PREFIX)-gcc $^ $(CFLAG) -o output/obj/$(notdir $@)
# 	$(PREFIX)-gcc $^ -E $(INC_DIR) -o output/obj/$(notdir $@).i

# --------------------------------------------------------------------------
# Clean
# --------------------------------------------------------------------------
clean :
	del /s /Q output\obj\*
	del /s /Q output\ld*
	del /s /Q output\tmp*
	del /s /Q output\*
