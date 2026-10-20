#include "cpu.h"
#include <stdio.h>

int main() {
    CPU cpu;
    cpu_init(&cpu);
    
    // 简单测试：addi x1, x0, 5
    cpu.memory[0] = 0x00500093;
    // ecall
    cpu.memory[1] = 0x00000073;
    
    printf("开始执行...\n");
    cpu_run(&cpu, 10);
    
    return 0;
}
