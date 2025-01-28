@echo off
setlocal ENABLEDELAYEDEXPANSION

set jlink="C:\Program Files (x86)\SEGGER\JLink\JLink.exe"

::--------------------------------------------------------
::-- %1: shell command file
::--------------------------------------------------------
if "%1" == "" (
	@echo Usage:
	@echo     %~nx0 command_file
	@echo Example:
	@echo     %~nx0 jlink_adfu.txt
	goto :eof
)

%jlink% -device Cortex-M33 -if SWD -speed 10000 -autoconnect 1 -CommandFile %1

::pause
