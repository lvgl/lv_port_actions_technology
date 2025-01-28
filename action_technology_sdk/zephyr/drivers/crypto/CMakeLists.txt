# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(
    ${MBEDTLS_DIR}/mbedtls/include
    ${MBEDTLS_DIR}/mbedtls/library
)

zephyr_library()
zephyr_library_sources_ifdef(
    CONFIG_AES_ACTS
    aes_sw.c
    aes_hw_acts.c
)

zephyr_library_sources_ifdef(
    CONFIG_SHA_ACTS
    sha1_sw.c
    sha256_sw.c
    sha_hw_acts.c
)

zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)
