@echo off
setlocal ENABLEDELAYEDEXPANSION

set ROOT_DIR=%~dp0
set BIN_FILE=%1
set OUT_DIR=%~dp1
set ZEPHYR_ELF=%OUT_DIR%\zephyr.elf

if "%BIN_FILE%" == "" (
	echo usage: %0 bin_file
	goto :eof
)

if not exist "%ZEPHYR_ELF%" (
	echo Error: please copy zephyr.elf to current directory
	pause
	goto :eof
)

python "%ROOT_DIR%\tracedump.py" "%ZEPHYR_ELF%" "%BIN_FILE%" "%OUT_DIR%\trace_dump.txt"

pause