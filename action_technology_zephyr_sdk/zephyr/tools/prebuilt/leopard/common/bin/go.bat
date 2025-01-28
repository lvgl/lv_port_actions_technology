 @echo off

set jlink_path="D:\SEGGER\JLink"

:: download rom
%jlink_path%\JLink.exe -autoconnect 1 -device  LEOPARD_FPGA  -if swd -speed 4000 -commandfile .\download_rom.jlink

::run
%jlink_path%\JLink.exe -autoconnect 1 -device  LEOPARD_FPGA  -if swd -speed 4000 -commandfile .\go.jlink
