@echo off

if "%KEIL_DIR%" == "" (
	set "KEIL_DIR=C:\Keil_v5"
)

:: Build Project
"%KEIL_DIR%\UV4\UV4.exe" -r mdk_clang.uvprojx -o output.txt
