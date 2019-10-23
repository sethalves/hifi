#
#  Copyright 2019 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

macro(TARGET_VIDEO_CODEC_SDK)
    if (VCPKG_CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(VIDEO_CODEC_SDK_INCLUDE_DIRS "${VCPKG_INSTALL_ROOT}/include/video-codec-sdk")
        find_library(VIDEO_CODEC_SDK_LIBRARY NAMES VideoCodecSDK PATHS ${VCPKG_INSTALL_ROOT}/lib/ NO_DEFAULT_PATH)
        target_include_directories(${TARGET_NAME} SYSTEM PUBLIC ${VIDEO_CODEC_SDK_INCLUDE_DIRS})
        target_link_libraries(${TARGET_NAME} ${VIDEO_CODEC_SDK_LIBRARY})
    endif()
endmacro()
