if(CONFIG_GLYPHIX)

set(ZEPHYR_CURRENT_LIBRARY glyphix_bsp)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(glyphix_bsp INTERFACE)

file(GLOB glyphix_header_file ${CMAKE_CURRENT_LIST_DIR}/include/*.h)

zephyr_include_directories(
	${CMAKE_CURRENT_LIST_DIR}/include
	${CMAKE_CURRENT_LIST_DIR}/src
	${CMAKE_CURRENT_LIST_DIR}/src/lvgx
	${CMAKE_CURRENT_LIST_DIR}/src/port
)

zephyr_interface_library_named(glyphix)
zephyr_library()

zephyr_compile_definitions(GX_PLATFORM_CONFIG)

FILE(GLOB_RECURSE SOURCES
	${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/*.c
)
zephyr_library_sources(${SOURCES})

zephyr_library_import(glyphix-core ${CMAKE_CURRENT_LIST_DIR}/lib/cortex-m33_gnu/libglyphix-core.a)
zephyr_library_import(quickjs ${CMAKE_CURRENT_LIST_DIR}/lib/cortex-m33_gnu/libquickjs.a)
zephyr_library_import(tjpgd ${CMAKE_CURRENT_LIST_DIR}/lib/cortex-m33_gnu/libtjpgd.a)
zephyr_library_import(glyphix-service ${CMAKE_CURRENT_LIST_DIR}/lib/cortex-m33_gnu/libglyphix-service.a)
zephyr_library_import(glyphix-platform ${CMAKE_CURRENT_LIST_DIR}/lib/cortex-m33_gnu/libglyphix-platform.a)
zephyr_library_import(glyphix-reactive ${CMAKE_CURRENT_LIST_DIR}/lib/cortex-m33_gnu/libglyphix-reactive.a)
zephyr_library_import(glyphix-widgets ${CMAKE_CURRENT_LIST_DIR}/lib/cortex-m33_gnu/libglyphix-widgets.a)
zephyr_library_import(glyphix-lodepng ${CMAKE_CURRENT_LIST_DIR}/lib/cortex-m33_gnu/libglyphix-lodepng.a)
zephyr_library_import(qrcode ${CMAKE_CURRENT_LIST_DIR}/lib/cortex-m33_gnu/libqrcode.a)
zephyr_library_import(cJSON ${CMAKE_CURRENT_LIST_DIR}/lib/cortex-m33_gnu/libcJSON.a)
zephyr_library_import(FlashDB ${CMAKE_CURRENT_LIST_DIR}/lib/cortex-m33_gnu/libFlashDB.a)
zephyr_library_import(tinycrypt ${CMAKE_CURRENT_LIST_DIR}/lib/cortex-m33_gnu/libtinycrypt.a)
zephyr_library_import(zlib ${CMAKE_CURRENT_LIST_DIR}/lib/cortex-m33_gnu/libzlib.a)

# extra modules
zephyr_library_import(glyphix-extra-mapview ${CMAKE_CURRENT_LIST_DIR}/lib/cortex-m33_gnu/libglyphix-extra-mapview.a)

target_link_libraries(glyphix_bsp INTERFACE zephyr_interface)

zephyr_library_link_libraries(posix_subsys mbedTLS)
endif()
