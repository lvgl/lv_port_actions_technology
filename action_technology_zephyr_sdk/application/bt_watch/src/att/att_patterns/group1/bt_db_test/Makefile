ATT_ROOT := $(CURDIR)/../../..
include $(ATT_ROOT)/att_patterns/scripts/config.mk

TARGET ?= p01_bta.$(OUTBINSUFFIX)

all:
#	cd $(IC_TYPE) && make || exit "$$?"
	cd lark && make || exit "$$?"
#	$(CPF) $(IC_TYPE)/$(TARGET) ../../../att_bin/outbin
	$(CPF) lark/$(TARGET) ../../../att_bin/outbin

clean:
#	cd $(IC_TYPE) && make clean || exit "$$?"
	cd lark && make clean || exit "$$?"
	#rm -rf $(ATT_ROOT)/att_bin/outbin/$(TARGET)
