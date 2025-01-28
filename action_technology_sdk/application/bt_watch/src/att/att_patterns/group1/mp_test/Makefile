ATT_ROOT := $(CURDIR)/../../..
include $(ATT_ROOT)/att_patterns/scripts/config.mk

TARGET ?= p05_rx.$(OUTBINSUFFIX)

all:
	cd $(IC_TYPE) && make || exit "$$?"
	$(CPF) $(IC_TYPE)/$(TARGET) ../../../att_bin/outbin
	#$(CPF) $(ATT_ROOT)/../../prebuild/bt_production_test/mp_btc.bin ../../../att_bin/

clean:
	cd $(IC_TYPE) && make clean || exit "$$?"
	#rm -rf $(ATT_ROOT)/att_bin/outbin/$(TARGET)
