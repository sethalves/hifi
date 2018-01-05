#
#  FindLZMA.cmake
# 
#  Try to find the LZMA library
#
#  Created on 2018-1-2 by Seth Alves
#  Copyright 2018 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

# setup hints for LZMA search
include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("lzma")

# locate header
find_path(LZMA_INCLUDE_DIRS "lzma.h" HINTS ${LZMA_SEARCH_DIRS})
find_library(LZMA_LIBRARIES NAMES lzma_lzma2_decoder_init PATH_SUFFIXES libs HINTS ${LZMA_SEARCH_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LZMA DEFAULT_MSG LZMA_INCLUDE_DIRS)

mark_as_advanced(LZMA_INCLUDE_DIRS LZMA_SEARCH_DIRS)
