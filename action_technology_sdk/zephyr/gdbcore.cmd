@echo off
setlocal ENABLEDELAYEDEXPANSION

::--------------------------------------------------------
::-- %1: coredump log
::--------------------------------------------------------
set "root=%~dp0"
set "core_log=%1"
set "core_bin=%~dp1\coredump.bin"
set "core_init=%root%\gdbcore.init"

:: check coredump log
if not exist "%core_log%" (
	@echo ERROR: coredump not exist
	pause
	goto :eof
)

:: check zephyr.elf
call :GET_OUTDIR outdir
set "core_elf=%root%\..\%outdir%\zephyr\zephyr.elf"
if not exist "%core_elf%" (
	@echo ERROR: zephyr.elf not exist "%core_elf%"
	pause
	goto :eof
)

:: generate coredump bin
python %root%\scripts\coredump\coredump_serial_log_parser.py "%core_log%" "%core_bin%"

:: start gdbserver
start cmd /c python %root%\scripts\coredump\coredump_gdbserver.py "%core_elf%" "%core_bin%"

:: start gdb
%ZEPHYR_TOOLS%\gcc-arm-none-eabi-9-2020-q2-update-win32\bin\arm-none-eabi-gdb "%core_elf%" -x "%core_init%"

goto:eof

::--------------------------------------------------------
::-- get outdir path
::-- %1: ret var
::--------------------------------------------------------
:GET_OUTDIR
	for /f "tokens=1-2 delims=\=" %%a in (%root%\.build_config) do (
		if "%%a" == "BOARD" (
			set board=%%~b
		)
		if "%%a" == "APPLICATION" (
			set app=%%~b
		)
	)
	set %1=application\%app%\outdir\%board%
goto:eof
