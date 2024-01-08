message("SYNAP_INCDIR:${SYNAP_INCDIR}")
message("SYNAP_LIBDIR:${SYNAP_LIBDIR}")

aux_source_directory(. SRC)
add_library(synap STATIC ${SRC})

target_link_libraries(synap PRIVATE
    ${SYNAP_LIBDIR}/libsynapnb.so
    ${SYNAP_LIBDIR}/libebg_utils.so)

target_include_directories(synap
    PUBLIC ${CMAKE_SOURCE_DIR}/include
    PUBLIC ${SYNAP_INCDIR}
    PRIVATE ${SYNAP_INCDIR}/ovxlib)