# SPDX-License-Identifier: Apache-2.0
#zephyr_include_directories(include)
zephyr_include_directories(app INTERFACE
	release/
	release/tuya_ble_app/
	release/tuya_ble_sdk/
	release/tuya_ble_sdk/app/product_test/
	release/tuya_ble_sdk/app/uart_common/
	release/tuya_ble_sdk/example/bulk_data/
	release/tuya_ble_sdk/port/
	release/tuya_ble_sdk/sdk/include/
	release/tuya_ble_sdk/sdk/lib/
	release/tuya_ble_sdk/extern_components/mbedtls/
	release/tuya_ble_sdk/modules/alexa/
	release/tuya_ble_sdk/modules/alexa/opus/
	release/tuya_ble_services/
)

add_subdirectory(release)
add_subdirectory(release/tuya_ble_app)
add_subdirectory(release/tuya_ble_services)
add_subdirectory(src/tuya_ble_sdk/app/product_test)
add_subdirectory(src/tuya_ble_sdk/app/uart_common)
add_subdirectory(src/tuya_ble_sdk/example/bulk_data)
add_subdirectory(src/tuya_ble_sdk/port)
add_subdirectory(src/tuya_ble_sdk/sdk/src)
add_subdirectory(src/tuya_ble_sdk/extern_components/mbedtls)
add_subdirectory(src/tuya_ble_sdk/modules/alexa)

target_link_libraries(app INTERFACE ${CMAKE_CURRENT_LIST_DIR}/libtuya_ble_secure.a)
#target_include_directories(app INTERFACE ${CMAKE_CURRENT_LIST_DIR}/release)
#target_include_directories(app INTERFACE ${CMAKE_CURRENT_LIST_DIR}/release/tuya_ble_app)
#target_include_directories(app INTERFACE ${CMAKE_CURRENT_LIST_DIR}/release/tuya_ble_sdk)
#target_include_directories(app INTERFACE ${CMAKE_CURRENT_LIST_DIR}/release/tuya_ble_sdk/app/product_test)
#target_include_directories(app INTERFACE ${CMAKE_CURRENT_LIST_DIR}/release/tuya_ble_sdk/app/uart_common)
#target_include_directories(app INTERFACE ${CMAKE_CURRENT_LIST_DIR}/release/tuya_ble_sdk/example/bulk_data)
#target_include_directories(app INTERFACE ${CMAKE_CURRENT_LIST_DIR}/release/tuya_ble_sdk/port)
#target_include_directories(app INTERFACE ${CMAKE_CURRENT_LIST_DIR}/release/tuya_ble_sdk/sdk/include)
#target_include_directories(app INTERFACE ${CMAKE_CURRENT_LIST_DIR}/release/tuya_ble_sdk/sdk/lib)
#target_include_directories(app INTERFACE ${CMAKE_CURRENT_LIST_DIR}/release/tuya_ble_sdk/extern_components/mbedtls)
#target_include_directories(app INTERFACE ${CMAKE_CURRENT_LIST_DIR}/release/tuya_ble_sdk/modules/alexa)
#target_include_directories(app INTERFACE ${CMAKE_CURRENT_LIST_DIR}/release/tuya_ble_sdk/modules/alexa/opus)
#target_include_directories(app INTERFACE ${CMAKE_CURRENT_LIST_DIR}/release/tuya_ble_services)

#target_sources_directory(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/release)
#target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/release/tuya_ble_app)
#target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/release/tuya_ble_services)
#target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)
#target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/tuya_ble_sdk)
#target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/tuya_ble_sdk/app/product_test)
#target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/tuya_ble_sdk/app/uart_common)
#target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/tuya_ble_sdk/example/bulk_data)
#target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/tuya_ble_sdk/port)
#target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/tuya_ble_sdk/sdk/src)
#target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/tuya_ble_sdk/extern_components/mbedtls)
#target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/tuya_ble_sdk/modules/alexa)

