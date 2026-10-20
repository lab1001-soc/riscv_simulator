@echo off
REM ============================================================
REM RISC-V Linux build script (Windows)
REM Run from tests\linux directory, or double-click
REM ============================================================

echo ============================================
echo   RISC-V Linux Build
echo ============================================

set TOOLCHAIN=C:\riscv_gcc\bin\riscv32-unknown-elf-

if not exist "%TOOLCHAIN%gcc.exe" (
    echo   ERROR: toolchain not found: %TOOLCHAIN%gcc.exe
    pause
    exit /b 1
)

REM ---- 1. SBI firmware ----
echo.
echo [1/4] Building SBI firmware...
pushd sbi
%TOOLCHAIN%gcc.exe -march=rv32imafc -mabi=ilp32f -nostdlib -T fw_stub.ld -o fw_stub.elf fw_stub.S
%TOOLCHAIN%objcopy.exe -O binary fw_stub.elf fw_stub.bin
copy /Y fw_stub.bin ..\fw_stub.bin >nul
popd
echo   Done: fw_stub.bin

REM ---- 2. rootfs (BusyBox via mkcpio2.py) ----
echo.
echo [2/4] Building rootfs...

pushd init
python mkcpio2.py . > ..\rootfs.cpio
popd
echo   Done: rootfs.cpio from busybox

:DTB
REM ---- 3. Device tree ----
echo.
echo [3/4] Generating DTB...
pushd DevTree
python mkdtb.py
copy /Y rv_sim.dtb ..\rv_sim.dtb >nul
popd
echo   Done: rv_sim.dtb

REM ---- 4. Kernel ----
echo.
echo [4/4] Checking kernel...

if exist "linux\Image" (
    copy /Y linux\Image Image >nul
    echo   Done: Image
) else if exist "Image" (
    echo   Image already exists
) else (
    echo   WARNING: Image missing, build on Ubuntu first
)

echo.
echo ============================================
echo   Build complete!
echo ============================================
echo.
dir fw_stub.bin rv_sim.dtb Image rootfs.cpio 2>nul
echo.
echo To run: cd ..\.. ^&^& make ^&^& rv_sim.exe -l
