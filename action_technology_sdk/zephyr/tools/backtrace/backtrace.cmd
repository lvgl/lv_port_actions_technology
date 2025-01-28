@echo off
setlocal ENABLEDELAYEDEXPANSION

set ROOT_DIR=%~dp0
set LOG_FILE=%1
set OUT_DIR=%~dp1
set ZEPHYR_ELF=%OUT_DIR%\zephyr.elf

if "%LOG_FILE%" == "" (
	echo usage: %0 log_file
	goto :eof
)

if not exist "%ZEPHYR_ELF%" (
	echo Error: please copy zephyr.elf to current directory
	pause
	goto :eof
)

python "%ROOT_DIR%\backtrace.py" "%LOG_FILE%" "%OUT_DIR%\backtrace.txt"

pause