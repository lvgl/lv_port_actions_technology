ATT_ROOT := $(CURDIR)/../..
include $(ATT_ROOT)/att_patterns/scripts/config.mk

TARGET ?= libatt.$(LIBSUFFIX)

SOURCES	:= $(wildcard *.c) $(wildcard ./rbuf/*.c) $(wildcard ./compensation/*.c)

all:
	cd $(IC_TYPE) && make || exit "$$?"

clean:
	cd $(IC_TYPE) && make clean || exit "$$?"
