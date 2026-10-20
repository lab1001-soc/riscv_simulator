# RV_sim Makefile - RISC-V RV32IMACFC 模拟器

CC = gcc
CFLAGS = -Wall -Wextra -O0 -g -std=c99 \
         -I. -Icpu -Iperipheral -Imem
LDFLAGS = -lm

TARGET = rv_sim.exe
ifeq (,$(findstring Windows,$(OS)))
TARGET = rv_sim          # Linux/macOS 宿主机
endif

# 源文件（相对于项目根目录）
SRCS = main.c \
       cpu/cpu.c cpu/decode.c cpu/execute.c cpu/fetch.c cpu/writeback.c \
       cpu/mmu.c \
       mem/mem.c \
       peripheral/uart.c peripheral/timer.c peripheral/software.c \
       peripheral/plic.c

OBJS = $(notdir $(SRCS:.c=.o))

# vpath 让 make 在子目录中查找源文件
vpath %.c cpu peripheral mem

.PHONY: all clean run test debug help

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(addprefix output/, $(OBJS)) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $(addprefix output/, $(@))

clean:
ifeq (,$(findstring Windows,$(OS)))
	rm -f $(TARGET) output/*.o *.o
else
	del -Q $(TARGET) output\*.o  *.o
endif

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	@echo "========================================"
	@echo "  运行 RV32IMACF 指令集测试"
	@echo "========================================"
	./$(TARGET)
	@echo ""
	@echo "========================================"
	@echo "  测试完成"
	@echo "========================================"

debug: CFLAGS += -DDEBUG_TRACE
debug: clean $(TARGET)

test_fpu: test_fpu.c
	$(CC) -o test_fpu test_fpu.c -lm
	./test_fpu

help:
	@echo "RV_sim - RISC-V RV32IMACF 模拟器"
	@echo ""
	@echo "构建选项:"
	@echo "  make        - 编译项目"
	@echo "  make run    - 编译并运行测试程序"
	@echo "  make test   - 运行完整测试"
	@echo "  make clean  - 清理构建文件"
	@echo "  make debug  - 编译调试版本 (带跟踪输出)"
	@echo "  make test_fpu - 编译并运行 FPU 测试生成器"
	@echo ""
	@echo "支持的指令集:"
	@echo "  - I: 基础整数指令 (ADD, SUB, AND, OR, 等)"
	@echo "  - M: 乘除扩展 (MUL, DIV, REM, 等)"
	@echo "  - A: 原子操作 (LR, SC, AMO, 等)"
	@echo "  - F: 单精度浮点 (FADD, FMUL, FMADD, 等)"
