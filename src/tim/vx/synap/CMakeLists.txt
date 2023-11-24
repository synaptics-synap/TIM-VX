if(${SYNAP_DIR})
    list(APPEND SYNAP_INC_DIRS ${SYNAP_DIR}/include)

    list(APPEND SYNAP_LIB_DIRS
        ${SYNAP_DIR}/lib/${PLATFORM}/libsynapnb.so
        ${SYNAP_DIR}/lib/${PLATFORM}/libebg_utils.so)
else()
    list(APPEND SYNAP_INC_DIRS ${SYNAPNB_INC} ${SYNAPBASE_INC} ${EBG_UTILS_INC})
    list(APPEND SYNAP_LIB_DIRS ${EBG_UTILS_LIB} ${SYNAPNB_LIB})
endif()

message("SYNAP_INC_DIRS:${SYNAP_INC_DIRS}")
message("SYNAP_LIB_DIRS:${SYNAP_LIB_DIRS}")

aux_source_directory(. SRC)
add_library(synap STATIC ${SRC})

target_link_libraries(synap PRIVATE ${SYNAP_LIB_DIRS})
target_include_directories(synap
    PUBLIC ${CMAKE_SOURCE_DIR}/include
    PUBLIC ${SYNAP_INC_DIRS}
    PRIVATE ${OVXLIB_INCLUDE_DIR})