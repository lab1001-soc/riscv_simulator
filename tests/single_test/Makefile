# 测试用例 Makefile

CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lm

# RV_sim 路径
RV_SIM = ../rv_sim

.PHONY: all clean run run_all

all: test_m.bin test_f.bin test_a.bin test_base.bin

# M 扩展测试
test_m.bin: test_m_extension.c
	$(CC) $(CFLAGS) -o test_m test_m_extension.c
	./test_m

# F 扩展测试
test_f.bin: test_f_extension.c
	$(CC) $(CFLAGS) -o test_f test_f_extension.c $(LDFLAGS)
	./test_f

# A 扩展测试
test_a.bin: test_a_extension.c
	$(CC) $(CFLAGS) -o test_a test_a_extension.c
	./test_a

# 基础整数测试
test_base.bin: test_base_integer.c
	$(CC) $(CFLAGS) -o test_base test_base_integer.c
	./test_base

# 运行单个测试
run_m: test_m.bin
	$(RV_SIM) test_m.bin

run_f: test_f.bin
	$(RV_SIM) test_f.bin

run_a: test_a.bin
	$(RV_SIM) test_a.bin

run_base: test_base.bin
	$(RV_SIM) test_base.bin

# 运行所有测试
run_all: all
	@echo "\n========================================"
	@echo "  运行所有测试用例"
	@echo "========================================\n"
	@echo "--- M 扩展测试 ---"
	$(RV_SIM) test_m.bin
	@echo "\n--- F 扩展测试 ---"
	$(RV_SIM) test_f.bin
	@echo "\n--- A 扩展测试 ---"
	$(RV_SIM) test_a.bin
	@echo "\n--- 基础整数测试 ---"
	$(RV_SIM) test_base.bin
	@echo "\n========================================"
	@echo "  全部测试完成"
	@echo "========================================"

clean:
	rm -f test_m test_f test_a test_base
	rm -f test_m.bin test_f.bin test_a.bin test_base.bin
