ATT_ROOT := $(CURDIR)/../../..
include $(ATT_ROOT)/att_patterns/scripts/config.mk

TARGET ?= p30_sn.$(OUTBINSUFFIX)

all:
	cd lark && make || exit "$$?"
	cp -rf lark/$(TARGET) $(ATT_ROOT)/att_bin/outbin

clean:
	cd lark && make clean || exit "$$?"
	#rm -rf $(ATT_ROOT)/att_bin/outbin/$(TARGET)
