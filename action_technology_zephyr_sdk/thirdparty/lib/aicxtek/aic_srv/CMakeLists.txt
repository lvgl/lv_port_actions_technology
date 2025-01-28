set(ZEPHYR_CURRENT_LIBRARY aic_srv_portable)

set(AIC_SRV_BASE ${CMAKE_CURRENT_LIST_DIR})

zephyr_include_directories(${AIC_SRV_BASE}/portable/include)
zephyr_include_directories(${AIC_SRV_BASE}/portable/tmr)
zephyr_include_directories(${AIC_SRV_BASE}/inc)

zephyr_library_named(aic_srv_portable)

FILE(GLOB src_list ${AIC_SRV_BASE}/portable/common/*.c)
target_sources(aic_srv_portable PRIVATE ${src_list})
FILE(GLOB src_list ${AIC_SRV_BASE}/portable/drivers/*.c)
target_sources(aic_srv_portable PRIVATE ${src_list})
FILE(GLOB src_list ${AIC_SRV_BASE}/portable/tmr/*.c)
target_sources(aic_srv_portable PRIVATE ${src_list})
FILE(GLOB src_list ${AIC_SRV_BASE}/portable/tele_cfg/*.c)
target_sources(aic_srv_portable PRIVATE ${src_list})
FILE(GLOB src_list ${AIC_SRV_BASE}/portable/log/*.c)
target_sources(aic_srv_portable PRIVATE ${src_list})
FILE(GLOB src_list ${AIC_SRV_BASE}/portable/net/*.c)
target_sources(aic_srv_portable PRIVATE ${src_list})

if (NOT EXISTS ${AIC_SRV_BASE}/portable/tools/sdk)
    add_definitions(-DCONFIG_TELE_SERVICE -DCONFIG_TELE_RADIO_SIM -DCONFIG_TELE_CALL -DCONFIG_TELE_DATA -DCONFIG_TELE_SMS -DCONFIG_TELE_SERVICE_TRANSACTION -DCONFIG_TELE_SAMPLE)
    target_link_libraries(aic_srv_portable PRIVATE ${AIC_SRV_BASE}/libs/libaic_srv.a)
endif()
