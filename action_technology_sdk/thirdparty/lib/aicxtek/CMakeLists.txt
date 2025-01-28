if (CONFIG_USING_AICXTEK_LIB)

if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/aic_srv)
add_subdirectory(aic_srv)
endif()

if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/portable)
add_subdirectory(portable)
endif()

endif()
