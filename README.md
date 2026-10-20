# RV_sim — RISC-V RV32IMACF 模拟器

纯 C 语言实现的 RISC-V 处理器模拟器，支持 **RV32IMACF** 完整指令集 + Zicsr 特权指令。可运行裸机测试、RT-Thread RTOS，以及 **Linux 6.1 + BusyBox 交互式 Shell**。

---

## 项目结构

```
RV_sim/
├── main.c                  # 主程序入口
├── config.h                # 全局配置
├── makefile                # Linux/GCC 构建
├── c.bat                   # Windows 快速编译脚本
├── r.bat                   # Windows 快速运行脚本
├── rv_sim.exe              # 编译产物（模拟器）
│
├── cpu/                    # CPU 核心
│   ├── cpu.h               #   CPU 结构体定义
│   ├── fetch.c             #   取指
│   ├── decode.c            #   指令译码
│   ├── execute.c           #   指令执行
│   ├── writeback.c         #   写回
│   └── mmu.c               #   Sv32 MMU（页表/TLB/地址翻译）
│
├── mem/                    # 内存模块
│   ├── mem.h               #   内存接口
│   └── mem.c               #   IMEM/DMEM/LMEM 实现
│
├── peripheral/             # 外设
│   ├── uart.c              #   NS16550A 串口
│   ├── timer.c             #   CLINT 定时器（mtime/mtimecmp）
│   ├── plic.c              #   PLIC 中断控制器
│   └── software.c          #   软件中断
│
└── tests/                  # 测试与固件
    ├── proj_test/          # ISA 全量测试（r 默认运行）
    ├── single_test/        # ISA 单项测试
    │
    ├── sdk/                # SDK（裸机固件开发套件）
    │   ├── libraries/
    │   │   ├── core/       #   基础库：hw.h（寄存器定义）、MyPrintf、types.h
    │   │   ├── sdk/        #   驱动库：uart.c、mtimer.c、software.c
    │   │   └── startup/    #   启动代码：startup.S（设置 gp/sp，跳转 main）
    │   └── demo/           #   示例工程
    │       ├── uart/       #     串口收发示例
    │       ├── timer/      #     定时器中断示例
    │       ├── software/   #     软件中断示例
    │       └── rt-thread/  #     RT-Thread Nano 移植
    │
    └── linux/              # Linux 启动文件
        ├── fw_stub.bin     # M-mode SBI 固件
        ├── rv_sim.dtb      # 设备树
        ├── rootfs.cpio     # initramfs 根文件系统（BusyBox）
        ├── Image           # Linux 内核镜像（~25MB）
        ├── sbi/            # fw_stub 源码
        ├── DevTree/        # 设备树源码 + 编译工具
        ├── busybox/        # BusyBox 源码
        └── rtos到linux修改方案.md  # Linux 启动调试记录（七轮 25 bug）
```

---

## 支持的指令集

RV_sim 完整实现 **RV32IMACF** 五个扩展 + 特权指令，共 200+ 条指令。

### I — 基础整数指令

- **算术**：`ADD`, `SUB`, `ADDI`
- **逻辑**：`AND`, `OR`, `XOR`, `ANDI`, `ORI`, `XORI`
- **移位**：`SLL`, `SRL`, `SRA`, `SLLI`, `SRLI`, `SRAI`
- **比较**：`SLT`, `SLTU`, `SLTI`, `SLTIU`
- **加载/存储**：`LB`, `LH`, `LW`, `LBU`, `LHU`, `SB`, `SH`, `SW`
- **跳转**：`JAL`, `JALR`
- **分支**：`BEQ`, `BNE`, `BLT`, `BGE`, `BLTU`, `BGEU`
- **上位立即数**：`LUI`, `AUIPC`
- **系统**：`ECALL`, `EBREAK`, `FENCE`

### M — 乘除扩展

- **乘法**：`MUL`, `MULH`, `MULHU`, `MULHSU`
- **除法**：`DIV`, `DIVU`
- **取余**：`REM`, `REMU`

### A — 原子操作扩展

- **加载保留/条件存储**：`LR.W`, `SC.W`
- **原子内存操作**：`AMOSWAP.W`, `AMOADD.W`, `AMOXOR.W`, `AMOAND.W`, `AMOOR.W`, `AMOMIN.W`, `AMOMAX.W`, `AMOMINU.W`, `AMOMAXU.W`

### C — 压缩指令扩展（16 位）

16 位压缩指令在执行时自动展开为等价的 32 位指令：

- **算术/逻辑**：`C.ADD`, `C.ADDI`, `C.SUB`, `C.XOR`, `C.OR`, `C.AND`
- **立即数**：`C.LI`, `C.LUI`, `C.ADDI16SP`, `C.ADDI4SPN`
- **移位**：`C.SLLI`, `C.SRLI`, `C.SRAI`, `C.ANDI`
- **加载/存储**：`C.LW`, `C.SW`, `C.FLW`, `C.FSW`, `C.LWSP`, `C.SWSP`, `C.FLWSP`, `C.FSWSP`
- **跳转/分支**：`C.J`, `C.JAL`, `C.JR`, `C.JALR`, `C.BEQZ`, `C.BNEZ`
- **其他**：`C.MV`, `C.NOP`, `C.EBREAK`

> RV64 专用的 `C.ADDW`/`C.SUBW`/`C.ADDIW` 在 RV32 中为非法指令，译码阶段直接拒绝。

### F — 单精度浮点扩展

- **算术**：`FADD.S`, `FSUB.S`, `FMUL.S`, `FDIV.S`, `FSQRT.S`
- **符号注入**：`FSGNJ.S`, `FSGNJN.S`, `FSGNJX.S`
- **比较**：`FMIN.S`, `FMAX.S`, `FEQ.S`, `FLT.S`, `FLE.S`
- **转换**：`FCVT.W.S`, `FCVT.WU.S`, `FCVT.S.W`, `FCVT.S.WU`
- **搬运**：`FMV.X.W`, `FMV.W.X`
- **分类**：`FCLASS.S`
- **加载/存储**：`FLW`, `FSW`

> 浮点寄存器 `f0-f31`，状态寄存器 `fcsr`（含 `fflags` 标志位 + `frm` 舍入模式）。

### 特权指令（跑 Linux 的关键）

- **CSR 访问**：`CSRRW`, `CSRRS`, `CSRRC`, `CSRRWI`, `CSRRSI`, `CSRRCI`
- **特权级返回**：`MRET`, `SRET`（M/S/U 三态，mideleg/medeleg 中断委托）
- **地址同步**：`SFENCE.VMA`（TLB 失效）
- **等待中断**：`WFI`
- **Sv32 MMU**（`cpu/mmu.c`）：页表遍历、TLB 缓存、页错误异常

---

## 内存布局

```
0x00008200  UART               串口（16550 兼容 + 自定义 RX/IER 寄存器）
0x00008300  CLINT Timer        mtime / mtimecmp
0x00008400  Software IRQ       MSIP 软件中断
0x00010000  DMEM (1MB)         数据存储器
0x01000000  IMEM (512KB)       指令存储器（裸机 PC 复位值 0x01000080）
0x0C000000  PLIC               中断控制器（53 个中断源，对齐 QEMU virt）
0x80000000  LMEM (128MB)       Linux 物理内存
```

Linux 模式（`r l`）的加载地址：

```
0x01000000  fw_stub.bin    M-mode SBI 固件（复位入口）
0x01001000  rv_sim.dtb     设备树
0x80000000  Image          Linux 内核（~25MB）
0x83000000  rootfs.cpio    initramfs（BusyBox）
```

---

## 使用方法

### 编译模拟器

```bat
c             :: 编译模拟器（默认）
c d           :: 编译调试版（带指令 trace）
```

### 运行测试

```bat
r             :: 编译并运行 proj_test（ISA 全量测试）
r t uart      :: 运行 SDK demo（uart/timer/software/rt-thread 等）
```

### 运行 Linux

```bat
c             :: 先编译模拟器
r l           :: 启动 Linux + BusyBox
```

`r l` 等价于在项目根目录执行 `rv_sim -l`（内核文件按相对路径 `tests/linux/` 加载，
必须从项目根目录运行）。启动后出现 `~ #` 交互式 shell，可执行 `ls`、`cat`、
`uname -a` 等命令。

### c 完整命令

| 命令                 | 说明                                         |
| -------------------- | -------------------------------------------- |
| `c`                | 编译模拟器（`make clean && make -j8`）     |
| `c d`              | 编译调试版（`make debug`，输出指令 trace） |
| `c t`              | 编译测试程序 `tests/proj_test`             |
| `c t <demo>`       | 编译 SDK demo（如 `c t uart`）             |
| `c t <demo> clean` | 清理后重新编译指定 demo                      |
| `c ta`             | 编译测试 + 模拟器（全量编译）                |
| `c ta <demo>`      | 编译指定 demo + 模拟器                       |

### r 完整命令

| 命令           | 说明                                                              |
| -------------- | ----------------------------------------------------------------- |
| `r`          | 运行 `tests/proj_test/output/out.bin`（不存在则自动编译）       |
| `r t <demo>` | 运行 `tests/sdk/demo/<demo>/output/out.bin`（不存在则自动编译） |
| `r l`        | Linux 仿真：加载 `tests/linux/` 下的固件/内核/设备树/根文件系统 |

---

## Linux 启动文件说明

`r l`（`rv_sim -l`）从 `tests/linux/` 加载以下 4 个文件：

| 文件            | 加载地址               | 说明                                        |
| --------------- | ---------------------- | ------------------------------------------- |
| `fw_stub.bin` | `0x01000000`（IMEM） | M-mode SBI 固件，负责中断委托、SBI 调用处理 |
| `rv_sim.dtb`  | `0x01001000`（IMEM） | 设备树，描述 CPU/内存/外设/中断拓扑         |
| `Image`       | `0x80000000`（LMEM） | Linux 内核镜像（~25MB）                     |
| `rootfs.cpio` | `0x83000000`（LMEM） | initramfs 根文件系统（含 BusyBox、init）    |

### 启动流程

```
CPU 复位 → fw_stub.bin (M-mode)
  → 配置 mideleg/medeleg（中断委托到 S-mode）
  → mret → S-mode → 执行 Linux Image
  → 解析 rv_sim.dtb → 初始化 MMU → 挂载 rootfs.cpio
  → execve("/init") → BusyBox shell
```

### SBI 接口

fw_stub 同时实现 **SBI v0.2** 扩展接口与 **Legacy** 接口：

| 接口             | a7 (EID)       | 说明                               |
| ---------------- | -------------- | ---------------------------------- |
| v0.2 Base        | `0x10`       | `probe` 查询扩展可用性、规范版本 |
| v0.2 Timer       | `0x54494D45` | `set_timer` 设置 mtimecmp        |
| Legacy set_timer | 0              | 设置 mtimecmp                      |
| Legacy putchar   | 1              | a0 = 字符                          |
| Legacy getchar   | 2              | 返回字符或 -1                      |
| Legacy shutdown  | 8              | WFI 死循环                         |

---

## SDK 固件开发

### 编写裸机程序

1. 在 `tests/sdk/demo/` 下创建新目录
2. 写 `main.c`，include `hw.h` + 驱动头文件
3. 复制已有 demo 的 makefile
4. `c t <你的demo>` → 生成 `output/out.bin`
5. `r t <你的demo>` 运行

---

## RT-Thread 移植

RT-Thread Nano 位于 `tests/sdk/demo/rt-thread/rtthread-nano/`：

- **内核**：线程调度、IPC、内存管理
- **CPU 移植**：`libcpu/risc-v/rv_sim/`（上下文切换、中断）
- **BSP**：`bsp/rv_sim/`（板级初始化）
- **FinSH Shell**：交互式命令行

运行：`r t rt-thread`，启动后出现 `msh >` 提示符。

---

## 开发历程（100+ 次提交，2026-05 ~ 2026-10）

| 阶段            | 时间       | 内容                                                          |
| --------------- | ---------- | ------------------------------------------------------------- |
| 基础 ISA        | 2026-05~06 | RV32I/M/A/C/F 完整指令集，ISA 测试 68/68 通过（截至 2026-10） |
| 外设与调试      | 2026-07    | Timer/PLIC/Software 中断、断点单步、模块化重构                |
| SDK + RT-Thread | 2026-07    | SDK 库 + 裸机 demo，RT-Thread Nano 移植                       |
| S-mode + MMU    | 2026-07    | 特权级、中断委托、Sv32 MMU（页表/TLB/页错误）                 |
| Linux 启动      | 2026-07~10 | 七轮调试、25 个 bug                                           |

---

## 许可证

MIT License
