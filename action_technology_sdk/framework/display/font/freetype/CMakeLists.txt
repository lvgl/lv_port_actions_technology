# SPDX-License-Identifier: Apache-2.0

SET(LIB_NAME freetype)
SET(ZEPHYR_CURRENT_LIBRARY ${LIB_NAME})
SET(LIBRARY_OUTPUT_PATH "${CMAKE_CURRENT_LIST_DIR}/../../")

zephyr_include_directories(./)
zephyr_include_directories(./builds/)
zephyr_include_directories(./include) 
zephyr_library_named(${LIB_NAME})

zephyr_library_compile_definitions(FT2_BUILD_LIBRARY)
zephyr_library_compile_options(-DCC_${LIB_NAME} -Wno-error -O3)

target_include_directories(${LIB_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/builds ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${LIB_NAME} INTERFACE zephyr_interface)

zephyr_library_include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/builds
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src/autofit
  ${CMAKE_CURRENT_SOURCE_DIR}/src/base
#  ${CMAKE_CURRENT_SOURCE_DIR}/src/cache
#  ${CMAKE_CURRENT_SOURCE_DIR}/src/cff
#  ${CMAKE_CURRENT_SOURCE_DIR}/src/psaux
#  ${CMAKE_CURRENT_SOURCE_DIR}/src/pshinter
#  ${CMAKE_CURRENT_SOURCE_DIR}/src/psnames
  ${CMAKE_CURRENT_SOURCE_DIR}/src/raster
  ${CMAKE_CURRENT_SOURCE_DIR}/src/sfnt
  ${CMAKE_CURRENT_SOURCE_DIR}/src/smooth
  ${CMAKE_CURRENT_SOURCE_DIR}/src/truetype
)

zephyr_library_sources(
  builds/zephyr/ftsystem.c
  builds/zephyr/ftqsort.c
  builds/zephyr/freetype_font_api.c

  src/base/ftbase.c
  src/base/ftinit.c
  src/base/ftdebug.c
  src/base/ftbbox.c
  src/base/ftglyph.c
  src/base/ftbitmap.c
#  src/cache/ftcache.c
#  src/cff/cff.c
#  src/psaux/psaux.c
#  src/pshinter/pshinter.c
#  src/psnames/psnames.c
  src/raster/raster.c
  src/sfnt/sfnt.c
  src/smooth/smooth.c
  src/truetype/truetype.c
)
