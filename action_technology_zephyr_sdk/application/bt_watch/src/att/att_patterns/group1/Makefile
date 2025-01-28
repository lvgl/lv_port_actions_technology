.PHONY:  bqb_test  bqb_test bt_db_test bt_db_read btstack_test channel_test gpio_test mp_test pwr_test product_test serialnumber_test clean

#all:  bt_db_test bt_db_read btstack_test channel_test gpio_test mp_test pwr_test product_test bqb_test
#all:  bt_db_test bt_db_read serialnumber_test bqb_test mp_test
all: bt_db_test bt_db_read serialnumber_test mp_test

bqb_test:
	cd ./bqb_test && make || exit "$$?"

bt_db_test:
	cd ./bt_db_test && make || exit "$$?"

bt_db_read:
	cd ./bt_db_read && make || exit "$$?"

btstack_test:
	cd ./btstack_test && make || exit "$$?"

serialnumber_test:
	cd ./serialnumber_test && make || exit "$$?"	

channel_test:
#	cd ./channel_test && make || exit "$$?"

gpio_test:
#	cd ./gpio_test && make || exit "$$?"

mp_test:
	cd ./mp_test && make || exit "$$?"

pwr_test:
#	cd ./pwr_test && make || exit "$$?"

product_test:
#	cd ./product_test && make || exit "$$?"

clean:
#	cd ./bqb_test && make clean
	cd ./bt_db_test && make clean
	cd ./bt_db_read && make clean
#	cd ./btstack_test && make clean
#	cd ./channel_test && make clean
#	cd ./gpio_test && make clean
	cd ./mp_test && make clean
#	cd ./pwr_test && make clean
#	cd ./product_test && make clean
	cd ./serialnumber_test && make clean