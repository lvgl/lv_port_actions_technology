@echo off

@echo make config.dat.c config.h

md out

cd out

type ..\..\config.txt.h ..\..\config.txt.c > config.txt

..\..\..\tools\config_maker\cfg_source.exe config.txt config.h config.dat.c

@echo make config.xml.c

..\..\..\tools\config_maker\xml_source.exe config.txt config.xml.c

