ATT_ROOT := $(CURDIR)/../../..
include $(ATT_ROOT)/att_patterns/scripts/config.mk

TARGET ?= p02_bta.$(OUTBINSUFFIX)

all:
	rm -rf link.ld
	cd lark && make || exit "$$?"
	$(CPF) lark/$(TARGET) ../../../att_bin/outbin

clean:
	cd lark && make clean || exit "$$?"
	#rm -rf $(ATT_ROOT)/att_bin/outbin/$(TARGET)
