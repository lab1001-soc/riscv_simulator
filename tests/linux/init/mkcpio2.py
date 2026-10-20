#!/usr/bin/env python3
"""mkcpio2.py — multi-file cpio newc packer with login support"""
import os, sys

def be32(v): return b'%08X' % v

def entry(name, data, mode=0o100755):
    namesize = len(name) + 1
    filesize = len(data)
    hdr = b'070701'
    for f in [0, mode, 0, 0, 1, 0, filesize, 0, 0, 0, 0, namesize, 0]:
        hdr += be32(f)
    hdr += name.encode() + b'\0'
    hdr += b'\0' * ((4 - len(hdr) % 4) % 4)
    hdr += data
    hdr += b'\0' * ((4 - len(data) % 4) % 4)
    return hdr

def dir_entry(name):
    namesize = len(name) + 1
    hdr = b'070701'
    for f in [0, 0o40755, 0, 0, 2, 0, 0, 0, 0, 0, 0, namesize, 0]:
        hdr += be32(f)
    hdr += name.encode() + b'\0'
    hdr += b'\0' * ((4 - len(hdr) % 4) % 4)
    return hdr

def symlink_entry(name, target):
    data = target.encode()
    namesize = len(name) + 1
    hdr = b'070701'
    for f in [0, 0o120777, 0, 0, 1, 0, len(data), 0, 0, 0, 0, namesize, 0]:
        hdr += be32(f)
    hdr += name.encode() + b'\0'
    hdr += b'\0' * ((4 - len(hdr) % 4) % 4)
    hdr += data
    hdr += b'\0' * ((4 - len(data) % 4) % 4)
    return hdr

def build(rootdir):
    out = b''
    # init 脚本：挂载基础文件系统后交给 BusyBox init（读 inittab 开 getty）
    # 直接用 BusyBox mount（Bug 19/22 修复 syscall 后已可用，
    # 旧 mount_proc 裸 syscall 绕过方案退役）
    init_script = b'''#!/bin/sh
export PATH=/bin:/sbin
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev
exec /sbin/init
'''
    # /etc/inittab：BusyBox init 的行为表。
    # getty 开在 ttyS0（8250 中断驱动，输入可用）；hvc0 只做内核 printk 输出
    # （hvc 的 khvcd 无 IRQ 时读不到数据会无限睡眠，输入不可靠——Bug 23）
    inittab = b'''::sysinit:/etc/init.d/rcS
ttyS0::respawn:/sbin/getty -L ttyS0 115200 vt100
::ctrlaltdel:/sbin/reboot
::shutdown:/bin/umount -a -r
'''
    # /etc/init.d/rcS：系统初始化脚本（挂载已在 /init 里由 mount_proc 完成）
    rcs = b'''#!/bin/sh
echo ""
echo "=============================="
echo "  rv_sim Linux + BusyBox"
echo "=============================="
echo ""
'''
    # /etc/passwd：root 用户，空密码
    passwd = b'root::0:0:root:/root:/bin/sh\n'
    # /etc/group
    group = b'root:x:0:\n'

    out += dir_entry('.')
    out += dir_entry('proc')
    out += dir_entry('sys')
    out += dir_entry('bin')
    out += dir_entry('sbin')
    out += dir_entry('dev')
    out += dir_entry('etc')
    out += dir_entry('root')
    out += entry('init', init_script, 0o100755)

    # busybox
    bb = open(os.path.join(rootdir, 'busybox'), 'rb').read()
    out += entry('bin/busybox', bb, 0o100755)

    # bin symlinks（login 给 getty 用，BusyBox getty 默认调 /bin/login）
    for cmd in ['sh', 'ls', 'cat', 'uname', 'ps', 'echo', 'mount',
                'sleep', 'dmesg', 'tail', 'login']:
        out += symlink_entry(f'bin/{cmd}', 'busybox')
    # sbin symlinks（login 供 getty 调用）
    for cmd in ['init', 'getty', 'login', 'reboot', 'ifconfig']:
        out += symlink_entry(f'sbin/{cmd}', '/bin/busybox')

    # etc files
    out += entry('etc/inittab', inittab, 0o100644)
    out += entry('etc/passwd', passwd, 0o100644)
    out += entry('etc/group', group, 0o100644)
    out += dir_entry('etc/init.d')
    out += entry('etc/init.d/rcS', rcs, 0o100755)

    out += entry('TRAILER!!!', b'', 0)
    out += b'\0' * ((512 - len(out) % 512) % 512)
    return out

if __name__ == '__main__':
    rootdir = sys.argv[1] if len(sys.argv) > 1 else '.'
    data = build(rootdir)
    sys.stdout.buffer.write(data)
    print(f"cpio: {len(data)} bytes", file=sys.stderr)
