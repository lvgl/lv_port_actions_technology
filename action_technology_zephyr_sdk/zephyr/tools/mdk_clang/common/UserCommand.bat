@echo off

::--------------------------------------------------------
::-- Input param
::-- %1: elf file
::-- %2: tool dir
::--------------------------------------------------------
set "ELF=%~dpnx1"
set "TMP=%~dp1\..\%~n1.tmp"
set "BIN=%~dp1\..\%~n1.bin"
set "LST=%~dp1\..\%~n1.lst"
set "TOOL=%~2\ARMCLANG\bin"

:: Output bin
"%TOOL%\fromelf" --bin --output="%TMP%" "%ELF%"
copy /B /Y "%TMP%\ER_VECTOR" "%BIN%" >nul 2>nul
rmdir /S /Q "%TMP%"

:: Output list
"%TOOL%\fromelf" --text -a -c --output="%LST%" "%ELF%"

:: Copy axf
copy /B /Y "%ELF%" .\ >nul 2>nul
