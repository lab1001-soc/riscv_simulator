@echo off
setlocal

:: Save project root directory
set "ROOT=%~dp0"

if /i "%~1"=="t" goto :run_sdk
if /i "%~1"=="l" goto :run_linux
goto :run_default

:: ------------------------------------------------------------------
:run_linux
cd /d "%ROOT%"
echo Running: Linux (fw_stub + Image + rootfs.cpio)
"%ROOT%rv_sim" -l
goto :eof

:: ------------------------------------------------------------------
:run_sdk
if "%~2"=="" goto :show_usage
if not exist "%ROOT%tests\sdk\demo\%2\makefile" goto :show_usage
goto :check_bin

:show_usage
echo Usage: r t ^<demo^>
echo        r l              ^(run Linux^)
echo.
echo Available demos:
for /d %%d in ("%ROOT%tests\sdk\demo\*") do echo   %%~nxd
goto :eof

:check_bin
if exist "%ROOT%tests\sdk\demo\%2\output\out.bin" goto :run_sdk_exec
call c t %2

:run_sdk_exec
echo Running: tests\sdk\demo\%2\output\out.bin
"%ROOT%rv_sim" "%ROOT%tests\sdk\demo\%2\output\out.bin"
goto :eof

:: ------------------------------------------------------------------
:run_default
if exist "%ROOT%tests\proj_test\output\out.bin" goto :run_default_exec
call c t

:run_default_exec
echo Running: tests\proj_test\output\out.bin
"%ROOT%rv_sim" "%ROOT%tests\proj_test\output\out.bin"
goto :eof
