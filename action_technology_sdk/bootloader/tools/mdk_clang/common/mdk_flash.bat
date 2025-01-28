@echo off

set "KEIL_DIR=C:\Keil_v5"

:: Flash Project
"%KEIL_DIR%\UV4\UV4.exe" -f mdk_clang.uvprojx -o output.txt
