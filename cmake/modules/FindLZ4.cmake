#
#  FindLZ4.cmake
# 
#  Try to find the LZ4 library
#
#  Created on 2018-1-2 by Seth Alves
#  Copyright 2018 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

# setup hints for LZ4 search
include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("lz4")

# locate header
find_path(LZ4_INCLUDE_DIRS "lz4.h" HINTS ${LZ4_SEARCH_DIRS})
find_library(LZ4_LIBRARIES NAMES LZ4_decompress_safe_partial PATH_SUFFIXES libs HINTS ${LZ4_SEARCH_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LZ4 DEFAULT_MSG LZ4_INCLUDE_DIRS)

mark_as_advanced(LZ4_INCLUDE_DIRS LZ4_SEARCH_DIRS)
