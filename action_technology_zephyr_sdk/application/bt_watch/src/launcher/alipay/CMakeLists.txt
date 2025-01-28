# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(
	.
)

target_sources(app PRIVATE
	alipay_ui.c
	alipay_bind_view.c
	alipay_pay_view.c
	alipay_unbind_view.c
	wxpay_bind_view.c
	wxpay_pay_view.c
	wxpay_unbind_view.c
	alipay_menu_view.c
	alipay_cardlist_view.c
	alipay_transitcode_view.c
)

if(CONFIG_ALIPAY_LIB)
	target_sources(app PRIVATE presenter_alipay.c)
	target_sources(app PRIVATE presenter_mock_wxpay.c)
elseif(CONFIG_ALIPAY_WXPAY_LIB)
	target_sources(app PRIVATE presenter_third_alipay.c)
	target_sources(app PRIVATE presenter_third_wxpay.c)
else()
	target_sources(app PRIVATE presenter_mock_alipay.c)
	target_sources(app PRIVATE presenter_mock_wxpay.c)
endif()
