ATT_ROOT := $(CURDIR)/../../../
include $(ATT_ROOT)/att_patterns/scripts/config.mk

IC_TYPE ?= leopard
ARCH ?= arm

LIB = libatt.$(LIBSUFFIX)

SOURCES	:= $(wildcard ../*.c) $(wildcard ..//rbuf/*.c) $(wildcard ..//compensation/*.c)
SOURCES	+= $(wildcard *.c) $(wildcard ./mp_btc/*.c) 

INCLUDES += -I$(ATT_ROOT)/att_patterns/include -I$(ATT_ROOT)/att_patterns/include/soc-include -I$(ATT_ROOT)/att_patterns/include/arm -I$(ATT_ROOT)/att_patterns/include/gcc-include
INCLUDES += -I$(ATT_ROOT)/att_patterns/include/sdk-include  -I$(ATT_ROOT)/ -I../

include $(ATT_ROOT)/att_patterns/scripts/$(IC_TYPE).mk
$(eval $(call MAKE_LIB,$(LIB),$(SOURCES)))
