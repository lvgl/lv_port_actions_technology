@echo off

@echo make config.bin

..\..\tools\config_maker\cfg_maker.exe out\config.dat.o out\defcfg.bin 33 0

..\..\tools\config_maker\xml_maker.exe out\config.xml.o out\config.xml ZS308A_V1.0

python ..\..\..\..\zephyr\scripts\config\config_xml_export.py -i out\config.xml -c out\config.txt -o out\drv_cfg_head.h

xcopy out\drv_cfg_head.h ..\..\..\..\zephyr\drivers\cfg_drv /F /y

xcopy out\defcfg.bin .\..\..\prebuild\config /F /y
xcopy out\defcfg.bin .\..\..\ksdfs /F /y

xcopy out\config.xml .\..\..\prebuild\config /F /y

xcopy out\config.h .\..\..\src\include /F /y


