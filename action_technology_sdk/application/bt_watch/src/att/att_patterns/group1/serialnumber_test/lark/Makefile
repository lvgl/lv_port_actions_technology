ATT_ROOT := $(CURDIR)/../../../../
include $(ATT_ROOT)/att_patterns/scripts/config.mk

TARGET := p30_sn

SOURCES := $(wildcard *.c) $(wildcard ../*.c)


INCLUDES += -I$(ATT_ROOT)/att_patterns/include -I$(ATT_ROOT)/att_patterns/include/soc-include -I$(ATT_ROOT)/att_patterns/include/arm -I$(ATT_ROOT)/att_patterns/include/gcc-include
INCLUDES += -I$(ATT_ROOT)/att_patterns/include/sdk-include  -I$(ATT_ROOT)/

LIBS += $(ATT_ROOT)/att_patterns/lib/$(IC_TYPE)/libatt.$(LIBSUFFIX)

include $(ATT_ROOT)/att_patterns/scripts/$(IC_TYPE).mk
$(eval $(call MAKE_PROG,$(TARGET),$(SOURCES),$(LDFILE)))

