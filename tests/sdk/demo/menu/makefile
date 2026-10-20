# RISC-V Menu Demo Makefile
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

INC_DIR  = -I ./
INC_DIR += -I ../../libraries/core
INC_DIR += -I ../../libraries/sdk

CFLAG  = -c -g -Os $(INC_DIR) -msmall-data-limit=0 -fno-toplevel-reorder \
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
SECTIONS
{

	__stack_size = DEFINED(__stack_size) ? __stack_size : 24K;
	. = 0x1000000;
	.text : { KEEP(*startup*) *(.text*) *(.rdata*) *(.rodata*) *(.srodata*) *(.eh_frame*) *(.srodata.cst*)}
	_sidata = .;
	. = 0x10060;
	.fixed : { _fixed = .; *(.fixed) }
	.data : AT (_sidata) { _sdata = .; *(.__bdfunc__*) *(.__bdpara__*) *malloc*(*.data *.bss)  *(.data*) *(.sdata*) }
	_edata = .;
	_gp = _sdata;
	.bss : { _sbss = .; *(.sbss*) *(.gnu.linkonce.sb.*) *(.bss .bss.*) *(COMMON) *(*.bss) *(*.scommon*)  }
	_ebss = .;

	. = ALIGN(8);
	_end = .;
	end = .;
	. = 0x110000 - __stack_size;
	_heap_end = . ;
	. = . + __stack_size;
	_stack = .;

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
all : output/a.out

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
	mkdir -p output/obj

# --------------------------------------------------------------------------
# Compile rules - explicit since sources live in different directories
# --------------------------------------------------------------------------
output/%.o : %.S
	$(PREFIX)-gcc $^ -o output/obj/$(notdir $@) $(CFLAG)

output/%.o : %.c
	$(PREFIX)-gcc $^ $(CFLAG) -o output/obj/$(notdir $@)

# --------------------------------------------------------------------------
# Clean
# --------------------------------------------------------------------------
clean :
	del /s /Q output\obj\*
	del /s /Q output\ld*
	del /s /Q output\tmp*
	del /s /Q output\*
