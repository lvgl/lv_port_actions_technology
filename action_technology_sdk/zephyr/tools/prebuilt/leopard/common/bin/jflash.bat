@echo off

set jlink_path=C:\Program Files (x86)\SEGGER\JLink
set jflash_path=%~dp0\..\..\..\..\..\..\zephyr\tools\jflash

:: download rom
::"%jlink_path%\JLink.exe" -autoconnect 1 -device  LEOPARD  -if swd -speed 4000 -commandfile .\download_rom.jlink

:: jflash bin to spinor（phy_adr+0x20000000）
"%jlink_path%\JFlash.exe" -openprj%jflash_path%\leopard.jflash -open.\mbrec.bin,0x20000000 -auto -exit
"%jlink_path%\JFlash.exe" -openprj%jflash_path%\leopard.jflash -open.\param.bin,0x20001000 -auto -exit
"%jlink_path%\JFlash.exe" -openprj%jflash_path%\leopard.jflash -open.\recovery.bin,0x20004000 -auto -exit
"%jlink_path%\JFlash.exe" -openprj%jflash_path%\leopard.jflash -open.\app.bin,0x20024000 -auto -exit
"%jlink_path%\JFlash.exe" -openprj%jflash_path%\leopard.jflash -open.\sdfs.bin,0x201db000 -auto -exit
"%jlink_path%\JFlash.exe" -openprj%jflash_path%\leopard.jflash -open.\nvram.bin,0x201eb000 -auto -exit
"%jlink_path%\JFlash.exe" -openprj%jflash_path%\leopard.jflash -open.\res.bin,0x20212000 -auto -exit
"%jlink_path%\JFlash.exe" -openprj%jflash_path%\leopard.jflash -open.\fonts.bin,0x20412000 -auto -exit
"%jlink_path%\JFlash.exe" -openprj%jflash_path%\leopard.jflash -open.\sdfs_k.bin,0x20a40000 -auto -exit
"%jlink_path%\JFlash.exe" -openprj%jflash_path%\leopard.jflash -open.\udisk.bin,0x20bd0000 -auto -exit

::run
"%jlink_path%\JLink.exe" -autoconnect 1 -device  LEOPARD  -if swd -speed 4000 -commandfile .\go.jlink
