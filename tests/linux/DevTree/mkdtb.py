#!/usr/bin/env python3
"""Generates a minimal FDT (Flattened Device Tree) binary for rv_sim."""

import os
import struct
import sys

def be32(v):
    return struct.pack('>I', v)

def pad4(data):
    """Pad data to 4-byte alignment."""
    while len(data) % 4 != 0:
        data += b'\0'
    return data

def fdt_begin_node(name):
    """FDT_BEGIN_NODE token followed by null-terminated name, 4-byte aligned."""
    return be32(0x00000001) + pad4(name.encode('utf-8') + b'\0')

def fdt_end_node():
    return be32(0x00000002)

def fdt_prop(nameoff, value):
    """FDT_PROP: len, nameoff, then padded value."""
    return be32(0x00000003) + be32(len(value)) + be32(nameoff) + pad4(value)

def fdt_end():
    return be32(0x00000009)

def build_fdt():
    # ---- Build strings block ----
    strings = []
    def add_str(s):
        if s in strings:
            return strings.index(s)  # no, we need the offset in the block
        # Actually, we need the byte offset
        pass

    # We'll collect strings and build the block at the end
    str_list = []
    str_offsets = {}
    def str_offset(s):
        """Register a string and return its offset in the strings block."""
        if s not in str_offsets:
            # Calculate offset if this string were added now
            offset = sum(len(x) + 1 for x in str_list)
            str_list.append(s)
            str_offsets[s] = offset
        return str_offsets[s]

    # Pre-register all property names
    str_offset("#address-cells")
    str_offset("#size-cells")
    str_offset("#interrupt-cells")
    str_offset("compatible")
    str_offset("bootargs")
    str_offset("timebase-frequency")
    str_offset("device_type")
    str_offset("reg")
    str_offset("reg-shift")
    str_offset("riscv,isa")
    str_offset("mmu-type")
    str_offset("clock-frequency")
    str_offset("stdout-path")
    str_offset("interrupt-controller")
    str_offset("linux,initrd-tstart")
    str_offset("linux,initrd-end")
    str_offset("phandle")
    str_offset("interrupt-parent")
    str_offset("interrupts")
    str_offset("interrupts-extended")
    str_offset("riscv,ndev")

    # ---- Build structure block ----
    struct_data = b''

    # Root node: /
    struct_data += fdt_begin_node("")

    # / properties
    struct_data += fdt_prop(str_offset("#address-cells"), be32(1))
    struct_data += fdt_prop(str_offset("#size-cells"), be32(1))
    struct_data += fdt_prop(str_offset("compatible"), pad4(b"rv_sim" + b'\0'))

    # /chosen node
    struct_data += fdt_begin_node("chosen")
    struct_data += fdt_prop(str_offset("bootargs"),
                            pad4(b"earlycon=sbi console=hvc0" + b'\0'))
    # stdout-path 已移除：避免内核把 uart@8200 注册成 console（与 getty 的
    # tty open 路径冲突）；printk 走 hvc0，串口交互走 ttyS0
    # initrd at 0x83000000 (above kernel, within LMEM)。
    # Bug 11: initrd-end 曾写成 0x84000000 (16MB)，但 rootfs.cpio 只有几 KB，
    # 内核解包器把剩余零数据全啃一遍 → soft lockup 刷屏 + 浪费 ~90s 墙钟。
    # end 必须等于 start + 实际文件大小（自动探测，失败时退回 5120）。
    initrd_start = 0x83000000
    try:
        initrd_end = initrd_start + os.path.getsize('../rootfs.cpio')
    except OSError:
        initrd_end = initrd_start + 0x1400   # 5120 字节
    struct_data += fdt_prop(str_offset("linux,initrd-start"), be32(initrd_start))
    struct_data += fdt_prop(str_offset("linux,initrd-end"),   be32(initrd_end))
    struct_data += fdt_end_node()

    # /cpus node
    struct_data += fdt_begin_node("cpus")
    struct_data += fdt_prop(str_offset("#address-cells"), be32(1))
    struct_data += fdt_prop(str_offset("#size-cells"), be32(0))
    struct_data += fdt_prop(str_offset("timebase-frequency"), be32(1000000))

    # /cpus/cpu@0 node
    struct_data += fdt_begin_node("cpu@0")
    struct_data += fdt_prop(str_offset("device_type"), pad4(b"cpu" + b'\0'))
    struct_data += fdt_prop(str_offset("reg"), be32(0))
    struct_data += fdt_prop(str_offset("compatible"), pad4(b"riscv" + b'\0'))
    struct_data += fdt_prop(str_offset("riscv,isa"), pad4(b"rv32imafc" + b'\0'))
    struct_data += fdt_prop(str_offset("mmu-type"), pad4(b"riscv,sv32" + b'\0'))
    struct_data += fdt_prop(str_offset("clock-frequency"), be32(1000000))

    # /cpus/cpu@0/interrupt-controller — riscv,cpu-intc (S-mode 软件中断1/定时器5/外部9)
    # 软件/定时器中断经 mideleg 委托到 S-mode；外部中断由 PLIC 汇聚到 cpu-intc 的 SEIP(9)
    struct_data += fdt_begin_node("interrupt-controller")
    struct_data += fdt_prop(str_offset("compatible"), pad4(b"riscv,cpu-intc" + b'\0'))
    struct_data += fdt_prop(str_offset("interrupt-controller"), b'')  # boolean, 空值
    struct_data += fdt_prop(str_offset("#interrupt-cells"), be32(1))
    struct_data += fdt_prop(str_offset("phandle"), be32(1))
    struct_data += fdt_end_node()  # cpu-intc

    struct_data += fdt_end_node()  # cpu@0

    struct_data += fdt_end_node()  # cpus

    # /memory@80000000 node
    struct_data += fdt_begin_node("memory@80000000")
    struct_data += fdt_prop(str_offset("device_type"), pad4(b"memory" + b'\0'))
    # reg = <0x80000000 0x08000000> = addr, size (128MB, 2 x 32-bit big-endian)
    struct_data += fdt_prop(str_offset("reg"), be32(0x80000000) + be32(0x08000000))
    struct_data += fdt_end_node()  # memory

    # /uart@8200 node (NS16550A compatible, reg-shift=2)
    # 中断线接 PLIC source 10（QEMU virt 同号），PLIC 汇聚到 cpu-intc SEIP
    struct_data += fdt_begin_node("uart@8200")
    struct_data += fdt_prop(str_offset("compatible"), pad4(b"ns16550a" + b'\0'))
    struct_data += fdt_prop(str_offset("reg"), be32(0x00008200) + be32(0x100))
    struct_data += fdt_prop(str_offset("reg-shift"), be32(2))
    struct_data += fdt_prop(str_offset("clock-frequency"), be32(1000000))
    struct_data += fdt_prop(str_offset("interrupt-parent"), be32(2))
    struct_data += fdt_prop(str_offset("interrupts"), be32(10))
    struct_data += fdt_end_node()  # uart

    # /soc/plic@0c000000 — SiFive PLIC（模拟器 peripheral/plic.c）
    # 单 S-mode context：输出线接 cpu-intc 的 S-mode external (9)
    struct_data += fdt_begin_node("plic@0c000000")
    struct_data += fdt_prop(str_offset("compatible"), pad4(b"sifive,plic-1.0.0" + b'\0'))
    struct_data += fdt_prop(str_offset("reg"), be32(0x0c000000) + be32(0x04000000))
    struct_data += fdt_prop(str_offset("interrupt-controller"), b'')
    struct_data += fdt_prop(str_offset("#interrupt-cells"), be32(1))
    struct_data += fdt_prop(str_offset("phandle"), be32(2))
    struct_data += fdt_prop(str_offset("interrupts-extended"), be32(1) + be32(9))
    struct_data += fdt_prop(str_offset("riscv,ndev"), be32(53))
    struct_data += fdt_end_node()  # plic

    struct_data += fdt_end_node()  # root
    struct_data += fdt_end()       # FDT_END

    # ---- Build strings block ----
    strings_data = b''
    for s in str_list:
        strings_data += s.encode('utf-8') + b'\0'

    # ---- Build memory reservation block ----
    mem_rsv = be32(0) + be32(0) + be32(0) + be32(0)  # no entries, terminator

    # ---- Assemble header ----
    off_mem_rsv = 40
    off_dt_struct = off_mem_rsv + len(mem_rsv)
    off_dt_strings = off_dt_struct + len(struct_data)
    totalsize = off_dt_strings + len(strings_data)

    header = b''
    header += be32(0xd00dfeed)         # magic
    header += be32(totalsize)          # totalsize
    header += be32(off_dt_struct)      # off_dt_struct
    header += be32(off_dt_strings)     # off_dt_strings
    header += be32(off_mem_rsv)        # off_mem_rsvmap
    header += be32(17)                 # version
    header += be32(16)                 # last_comp_version
    header += be32(0)                  # boot_cpuid_phys
    header += be32(len(strings_data))  # size_dt_strings
    header += be32(len(struct_data))   # size_dt_struct

    # ---- Combine ----
    fdt = header + mem_rsv + struct_data + strings_data

    # Pad to 8-byte alignment (not strictly required but good practice)
    while len(fdt) % 8 != 0:
        fdt += b'\0'

    return fdt

def main():
    fdt = build_fdt()
    out = sys.argv[1] if len(sys.argv) > 1 else "rv_sim.dtb"
    with open(out, 'wb') as f:
        f.write(fdt)
    print(f"Generated {out}: {len(fdt)} bytes")
    print(f"  FDT header: 0xd00dfeed")
    print(f"  Structure:   {len(fdt)} bytes total")

if __name__ == '__main__':
    main()
