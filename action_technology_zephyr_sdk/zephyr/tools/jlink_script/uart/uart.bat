@echo off
setlocal ENABLEDELAYEDEXPANSION

::--------------------------------------------------------
::-- %1: shell command file
::--------------------------------------------------------
if "%1" == "" (
	@echo Usage:
	@echo     %~nx0 command_file
	@echo Example:
	@echo     %~nx0 com_adfu.txt
	goto :eof
)

for /f "tokens=1-2 delims=: " %%a in ('MODE') do (
	set dev=%%b
	set pref=!dev:~0,3!
	if "!pref!" == "COM" (
		MODE !dev!:2000000,N,8
		for /f "tokens=*" %%a in (%1) do (
			echo %%a > !dev!
		)
	)
)

::pause
