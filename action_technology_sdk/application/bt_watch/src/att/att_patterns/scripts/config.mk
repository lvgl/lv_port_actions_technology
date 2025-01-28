ifeq ($(OS),Windows_NT)
POSIXSHELL :=
else
# not on windows:
POSIXSHELL := 1
endif

#$(CURDIR)是makefile内部变量，对于cygwin编译需要转换路径

CURPATH := $(abspath $(lastword $(MAKEFILE_LIST)))
PREFIX := $(findstring /cygdrive/, $(CURDIR))
ifeq ($(PREFIX), /cygdrive/) 
	CURDIR := $(shell cygpath -a -m $(CURDIR))
	#cygwin使用clang编译器
	POSIXSHELL :=  
endif
#$(info "CURDIR is $(CURDIR)")

ifneq ($(POSIXSHELL),)

CMDSEP := ;
PSEP := /
CPF := cp -f
# more variables for commands you need

TOOLCHAIN       := GCC

else

CMDSEP := &
PSEP := \\
CPF := cp
# more variables for commands you need
TOOLCHAIN       := CLANG

endif

#$(info "current shell is  $(POSIXSHELL)")
#$(info "current toolchain is $(TOOLCHAIN)")


# Build verbosity
V			:= 0
# Debug build
DEBUG			:= 1
# Build platform
PLAT			:= LEOPARD

IC_TYPE ?= leopard
ARCH ?= arm
BOARD ?= leopard_dvb

ifeq ($(TOOLCHAIN), GCC)
LIBSUFFIX=a
LDFILE = link.ld.S
OUTBINSUFFIX=bin
else
LIBSUFFIX=lib
LDFILE = link.sct.S
OUTBINSUFFIX=bin
endif