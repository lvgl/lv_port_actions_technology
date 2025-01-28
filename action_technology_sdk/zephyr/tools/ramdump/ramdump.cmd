@echo off
setlocal ENABLEDELAYEDEXPANSION

set ROOT_DIR=%~dp0
set TEMP_BIN=%1
set OUT_DIR=%~dpn1
set ZEPHYR_ELF=%OUT_DIR%\..\zephyr.elf

if "%TEMP_BIN%" == "" (
	echo usage: %0 ramdump_bin
	goto :eof
)

python "%ROOT_DIR%\ramdump.py" -d "%TEMP_BIN%" -o "%OUT_DIR%"

if not exist "%ZEPHYR_ELF%" (
	echo Please copy zephyr.elf to current directory
	pause
	goto :eof
)

for /d %%a in (%OUT_DIR%\*) do (
	set mtb_bin=%%a\0x31000000.bin
	set mtb_out=%%a\trace_mtb.txt
	if exist !mtb_bin! (
		python "%ROOT_DIR%\trace_mtb.py" -e "%ZEPHYR_ELF%" -b !mtb_bin! > !mtb_out!
		echo [Trace] !mtb_bin! -^> !mtb_out!
	)
	set trcd_bin=%%a\trace_dump.bin
	set trcd_out=%%a\trace_dump.txt
	if exist !trcd_bin! (
		python "%ROOT_DIR%\tracedump.py" "%ZEPHYR_ELF%" !trcd_bin! !trcd_out!
		echo [Trace] !trcd_bin! -^> !trcd_out!
	)
)

pause