@echo off
setlocal ENABLEDELAYEDEXPANSION

::--------------------------------------------------------
::-- %1: board-name
::-- %2: app-dir
::-- %3: cfg-dir
::-- %4: lib-name
::--------------------------------------------------------
if "%2" == "" (
	@echo Usage:
	@echo     %~nx0 board-name app-dir cfg-dir [lib-name]
	@echo Example:
	@echo     %~nx0 lark_dvb_watch samples\hello_world .
	goto :eof
)

set lib=
if not "%4" == "" (
	set lib=%4
)

set tc=clang
set board=%1
set adir=%~2
set cfgdir=%3
set cfgfile=%cfgdir:\=/%/prj.conf
set bdir=%~2\outdir\%board%\mdk_%tc%
set sdir=%cd%
set zdir=%sdir%
set mdk=%sdir%\tools\mdk_%tc%

rmdir /S /Q %bdir% >nul 2>nul
call "%zdir%\zephyr-env.cmd"

cmake -GNinja -DBOARD=%board% -H%adir% -B%bdir% -DCONF_FILE=%cfgfile%

call :GET_SOC_NAME soc_name
call :GET_SYSCALL_INCS syscall_incs
python %zdir%/scripts/parse_syscalls.py %syscall_incs% --json-file %bdir%/zephyr/misc/generated/syscalls.json --tag-struct-file %bdir%/zephyr/misc/generated/struct_tags.json

python %zdir%/scripts/gen_kobject_list.py --validation-output %bdir%/zephyr/include/generated/driver-validation.h --include %bdir%/zephyr/misc/generated/struct_tags.json

python %zdir%/scripts/gen_kobject_list.py --kobj-types-output %bdir%/zephyr/include/generated/kobj-types-enum.h --kobj-otype-output %bdir%/zephyr/include/generated/otype-to-str.h --kobj-size-output %bdir%/zephyr/include/generated/otype-to-size.h --include %bdir%/zephyr/misc/generated/struct_tags.json

python %zdir%/scripts/gen_syscalls.py --json-file %bdir%/zephyr/misc/generated/syscalls.json --base-output %bdir%/zephyr/include/generated/syscalls --syscall-dispatch %bdir%/zephyr/include/generated/syscall_dispatch.c --syscall-list %bdir%/zephyr/include/generated/syscall_list.h --split-type k_timeout_t

set offs=arch/arm/core/offsets/offsets.c
%GNUARMEMB_TOOLCHAIN_PATH%\bin\arm-none-eabi-gcc.exe ^
	-DKERNEL -D_FORTIFY_SOURCE=2 -D__PROGRAM_START -D__ZEPHYR__=1 ^
	-I%zdir%/kernel/include -I%zdir%/arch/arm/include -I%zdir%/include -I%bdir%/zephyr/include/generated -I%zdir%/drivers -I%zdir%/subsys/tracing/include -I%zdir%/subsys/tracing/sysview -I%sdir%/soc/arm/actions/leopard -I%sdir%/../thirdparty/hal/cmsis/CMSIS/Core/Include -I%zdir%/modules/segger -I%zdir%/../thirdparty/debug/segger/SEGGER -I%zdir%/../thirdparty/debug/segger/Config -I%sdir%/../thirdparty/debug/segger/systemview -isystem %zdir%/lib/libc/minimal/include -isystem %GNUARMEMB_TOOLCHAIN_PATH%/lib/gcc/arm-none-eabi/9.3.1/include ^
	-Os -imacros%bdir%/zephyr/include/generated/autoconf.h -ffreestanding -fno-common -g -mcpu=cortex-m33+nodsp -mthumb -mabi=aapcs -imacros%zdir%/include/toolchain/zephyr_stdint.h -Wall -Wformat -Wformat-security -Wno-format-zero-length -Wno-main -Wno-address-of-packed-member -Wno-pointer-sign -Wpointer-arith -Wno-unused-but-set-variable -Werror=implicit-int -fno-asynchronous-unwind-tables -fno-pie -fno-pic -fno-strict-overflow -fno-reorder-functions -fno-defer-pop  -ffunction-sections -fdata-sections -std=c99 -nostdinc ^
	-MD -MT %bdir%/zephyr/CMakeFiles/offsets.dir/%offs%.obj -MF %bdir%/zephyr/CMakeFiles/offsets.dir/%offs%.obj.d ^
	-o %bdir%/zephyr/CMakeFiles/offsets.dir/%offs%.obj -c %zdir%/%offs%

python %zdir%/scripts/gen_offset_header.py -i %bdir%/zephyr/CMakeFiles/offsets.dir/%offs%.obj -o %bdir%/zephyr/include/generated/offsets.h

copy /Y %mdk%\common\* %bdir%\
copy /Y %mdk%\soc\%soc_name%\* %bdir%\
if exist %mdk%\boards\%board% (
	copy /Y %mdk%\boards\%board%\* %bdir%
)
call :GEN_PACK_BAT %board% %adir% %bdir%
if exist %adir%\mdk_%tc% (
	copy /Y %adir%\mdk_%tc%\* %bdir%
)
if exist %adir%\%cfgdir%\mdk_%tc% (
	copy /Y %adir%\%cfgdir%\mdk_%tc%\* %bdir%
)

%sdir%\tools\ninja2mdk.exe %bdir%\build.ninja %bdir%\template.uvprojx %bdir%\mdk_%tc%.uvprojx %lib%

del /S /Q %bdir%\template.* >nul 2>nul
mkdir %bdir%\tmp
move /Y %bdir%\zephyr\.config %bdir%\tmp\ >nul 2>nul
echo /*empty file*/ > %bdir%\tmp\ksyms.S
move /Y %bdir%\zephyr\include %bdir%\tmp\ >nul 2>nul
rmdir /S /Q %bdir%\app %bdir%\src %bdir%\CMakeFiles %bdir%\Kconfig %bdir%\thirdparty %bdir%\zephyr >nul 2>nul
del /S /Q %bdir%\cmake_install.cmake %bdir%\CMakeCache.txt %bdir%\zephyr_modules.txt %bdir%\zephyr_settings.txt >nul 2>nul
move /Y %bdir%\tmp %bdir%\zephyr >nul 2>nul

goto:eof

::--------------------------------------------------------
::-- get soc name
::-- %1: ret var
::--------------------------------------------------------
:GET_SOC_NAME
	for /f "tokens=1-3 delims= " %%a in (%bdir%\zephyr\include\generated\autoconf.h) do (
		if "%%b" == "CONFIG_SOC_SERIES" (
			set %1=%%~c
		)
	)
goto:eof

::--------------------------------------------------------
::-- get syscalls incs
::-- %1: ret var
::--------------------------------------------------------
:GET_SYSCALL_INCS
	for /f "tokens=1-3 delims= " %%a in (%bdir%\zephyr\include\generated\autoconf.h) do (
		if "%%b" == "CONFIG_ZTEST" (
			set ztestinc=--include %zdir%/subsys/testsuite/ztest/include
		)
		if "%%b" == "CONFIG_TRACING" (
			set tracinginc=--include %zdir%/subsys/tracing/include
		)
		if "%%b" == "CONFIG_APPLICATION_DEFINED_SYSCALL" (
			set appinc=--include %adir%
		)
	)
	set %1=--include %zdir%/include --include %zdir%/drivers --include %zdir%/subsys/net %ztestinc% %appinc% %tracinginc%
goto:eof

::--------------------------------------------------------
::-- gen pack bat
::-- %1: board
::-- %2: app dir
::-- %3: build dir
::--------------------------------------------------------
:GEN_PACK_BAT
	set zbase=..\..\..\..\..\..\zephyr
	echo @echo off > %3\PackCommand.bat
	echo copy /Y /B zephyr.bin %zbase%\boards\arm\%1\%~n2.bin >> %3\PackCommand.bat
goto:eof