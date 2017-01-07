#
#  FindBinaryFaceHMD.cmake
#
#  Try to find the BinaryFaceHMD tracker library
#
#  You must provide a BINARYFACEHMD_ROOT_DIR which contains 3rdParty, include, and libs directories
#
#  Once done this will define
#
#  BINARYFACEHMD_FOUND - system found BinaryFaceHMD
#  BINARYFACEHMD_INCLUDE_DIRS - the BinaryFaceHMD include directory
#  BINARYFACEHMD_LIBRARIES - link this to use BinaryFaceHMD
#
#  Supports Win64 only 
# 
#  Created on 15 Nov 2016 by Jungwoon Park
#  Copyright 2016 High Fidelity, Inc. ???
#

if (WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 8)

    include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
    hifi_library_search_hints("BinaryFaceHMD")

    find_path(BINARYFACEHMD_INCLUDE_DIRS binaryfacehmd.h PATH_SUFFIXES include HINTS ${BINARYFACEHMD_SEARCH_DIRS})
    find_library(BINARYFACEHMD_LIBRARIES NAMES binaryfacehmd PATH_SUFFIXES windows/x86_64 HINTS ${BINARYFACEHMD_SEARCH_DIRS})
    find_path(BINARYFACEHMD_DLL_PATH binaryfacehmd.dll PATH_SUFFIXES windows/x86_64 HINTS ${BINARYFACEHMD_SEARCH_DIRS})
    find_path(ROYALE_DLL_PATH royale.dll PATH_SUFFIXES windows/x86_64 HINTS ${BINARYFACEHMD_SEARCH_DIRS})
    list(APPEND BINARYFACEHMD_REQUIREMENTS BINARYFACEHMD_INCLUDE_DIRS BINARYFACEHMD_LIBRARIES BINARYFACEHMD_DLL_PATH)

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(BINARYFACEHMD DEFAULT_MSG ${BINARYFACEHMD_REQUIREMENTS})

    add_paths_to_fixup_libs(${BINARYFACEHMD_DLL_PATH})
    add_paths_to_fixup_libs(${ROYALE_DLL_PATH})

    mark_as_advanced(BINARYFACEHMD_INCLUDE_DIRS BINARYFACEHMD_LIBRARIES BINARYFACEHMD_SEARCH_DIRS)

endif()
