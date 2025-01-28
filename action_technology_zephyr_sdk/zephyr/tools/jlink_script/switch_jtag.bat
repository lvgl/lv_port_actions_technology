@echo off
setlocal ENABLEDELAYEDEXPANSION

call uart\uart.bat uart\uart_jtag.txt
call jlink\jlink.bat jlink\jlink_jtag.txt
