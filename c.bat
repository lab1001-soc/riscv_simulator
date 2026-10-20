@echo off
setlocal

:: Save project root directory
set "ROOT=%~dp0"

if /i "%~1"=="t"  goto :build_test
if /i "%~1"=="ta" goto :build_all
goto :build_sim

:: ------------------------------------------------------------------
:: c t         Build proj_test
:: c t <demo>  Build tests/sdk/demo/<demo>
:: ------------------------------------------------------------------
:build_test
if not "%~2"=="" goto :build_test_sdk

:: c t  (build proj_test)
echo === Building tests/proj_test ===
cd /d "%ROOT%tests\proj_test"
make clean
make -j8
goto :eof

:build_test_sdk
if exist "%ROOT%tests\sdk\demo\%2\makefile" goto :build_test_sdk_ok
echo Error: demo "%~2" not found
call :list_demos
goto :eof

:build_test_sdk_ok
echo === Building tests/sdk/demo/%~2 ===
cd /d "%ROOT%tests\sdk\demo\%2"
if not "%~3"=="" make clean
make -j8
goto :eof

:: ------------------------------------------------------------------
:: c ta         Build proj_test + simulator
:: c ta <demo>  Build SDK demo + simulator
:: ------------------------------------------------------------------
:build_all
if "%~2"=="" (
    call "%0" t
) else (
    call "%0" t %2
)
echo === Building simulator ===
cd /d "%ROOT%"
make clean
make -j8
goto :eof

:: ------------------------------------------------------------------
:: Default: build simulator only
:: ------------------------------------------------------------------
:build_sim
make clean
if [%1] == [d] (
    make debug
) else (
    make -j8
)
goto :eof

:: ------------------------------------------------------------------
:: Print available SDK demos
:: ------------------------------------------------------------------
:list_demos
if exist "%ROOT%tests\sdk\demo" (
    echo Available demos:
    for /d %%d in ("%ROOT%tests\sdk\demo\*") do echo   %%~nxd
)
goto :eof
