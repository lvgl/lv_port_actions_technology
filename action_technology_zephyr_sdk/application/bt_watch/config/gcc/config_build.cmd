@echo off
setlocal ENABLEDELAYEDEXPANSION


set sdir=%cd%
set zdir=%sdir%\..\..\..\..\zephyr

set fdir=%sdir%\..\..\..\..\framework

set odir=%sdir%\out

rmdir /S /Q %odir% >nul 2>nul
call "%zdir%\zephyr-env.cmd"


call prepare.bat

@echo make config.dat.c

%GNUARMEMB_TOOLCHAIN_PATH%\bin\arm-none-eabi-gcc.exe ^
	-DKERNEL -D_FORTIFY_SOURCE=2 -D__PROGRAM_START -D__ZEPHYR__=1 ^
	-I%fdir%/system/include -I%odir%  -isystem %GNUARMEMB_TOOLCHAIN_PATH%/lib/gcc/arm-none-eabi/9.3.1/include ^
	-Os -ffreestanding -fno-common -g -mcpu=cortex-m4 -mthumb -mabi=aapcs -Wall -Wformat -Wformat-security -Wno-format-zero-length -Wno-main -Wno-address-of-packed-member -Wno-pointer-sign -Wpointer-arith -Wno-unused-but-set-variable -Werror=implicit-int -fno-asynchronous-unwind-tables -fno-pie -fno-pic -fno-strict-overflow -fno-reorder-functions -fno-defer-pop  -ffunction-sections -fdata-sections -std=c99 -nostdinc ^
	-o %odir%/config.dat.o -c %odir%/config.dat.c
	
@echo make config.xml.c
	
%GNUARMEMB_TOOLCHAIN_PATH%\bin\arm-none-eabi-gcc.exe ^
	-DKERNEL -D_FORTIFY_SOURCE=2 -D__PROGRAM_START -D__ZEPHYR__=1 ^
	-I%fdir%/system/include -I%odir%  -isystem %GNUARMEMB_TOOLCHAIN_PATH%/lib/gcc/arm-none-eabi/9.3.1/include ^
	-Os -ffreestanding -fno-common -g -mcpu=cortex-m4 -mthumb -mabi=aapcs -Wall -Wformat -Wformat-security -Wno-format-zero-length -Wno-main -Wno-address-of-packed-member -Wno-pointer-sign -Wpointer-arith -Wno-unused-but-set-variable -Werror=implicit-int -fno-asynchronous-unwind-tables -fno-pie -fno-pic -fno-strict-overflow -fno-reorder-functions -fno-defer-pop  -ffunction-sections -fdata-sections -std=c99 -nostdinc ^
	-o %odir%/config.xml.o -c %odir%/config.xml.c	


cd %sdir%
	
call post.bat
	

