if(NOT ${TIM_VX_ENABLE_SYNAP} AND NOT EXISTS ${SYNAP_DIR})
    message(FATAL_ERROR "not existing SYNAP_DIR: ${SYNAP_DIR}")
endif()

message("include SyNAP driver sdk from ${SYNAP_DIR}")

if(CMAKE_CROSSCOMPILING)
    if(ANDROID_PLATFORM)
        if(${ANDROID_ABI} MATCHES "^armeabi")
            set(TARGET_CPU armv7a)
        elseif(${ANDROID_ABI} MATCHES "arm64-v8a")
            set(TARGET_CPU aarch64)
        else()
            message(FATAL_ERROR "unsupport ANDROID_ABI:${ANDROID_ABI}")
        endif()

        set(PLATFORM ${TARGET_CPU}-android-ndk-api${ANDROID_PLATFORM_LEVEL})
    elseif(NOT PLATFORM)
        message(FATAL_ERROR "Unsupported platform")
    endif()
else()
    set(PLATFORM x86_64-linux-gcc)
endif()

message("TIM_VX_USE_EXTERNAL_OVXLIB:${TIM_VX_USE_EXTERNAL_OVXLIB}")

# only check for standalone tim-vx build,
# synap integration build always set this config
if(NOT ${TIM_VX_USE_EXTERNAL_OVXLIB})
    set(TIM_VX_USE_EXTERNAL_OVXLIB ON)
    set(TIM_VX_ENABLE_VIPLITE OFF)
    set(OVXLIB_LIB ${SYNAP_DIR}/lib/${PLATFORM}/libovxlib.so)
    set(OVXLIB_INC ${SYNAP_DIR}/include/ovxlib)
    set(OVXDRV_INCLUDE_DIRS
        ${SYNAP_DIR}/include
        ${SYNAP_DIR}/include/CL)
endif()
