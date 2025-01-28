@echo off
setlocal ENABLEDELAYEDEXPANSION

call uart\uart.bat uart\uart_adfu.txt
call jlink\jlink.bat jlink\jlink_adfu.txt
