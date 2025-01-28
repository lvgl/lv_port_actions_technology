ROOT := $(CURDIR)

V ?= 0
IC_TYPE = leopard

ifeq ($(IC_TYPE), leopard)
	ARCH := arm
else ifeq ($(IC_TYPE), woodpecker)
	ARCH := mips
else
	ARCH := csky
endif

LIB = libbt_test.a
CFLAGS	+=	-fno-strict-aliasing

SOURCES += $(wildcard *.S) $(wildcard *.c)

INCLUDES += -I.  -I./inc

include $(ROOT)/scripts/$(IC_TYPE).mk
$(eval $(call MAKE_LIB,$(LIB),$(SOURCES)))
